// Copyright 2013 by Raytheon BBN Technologies
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Urdu/tokens/ur_Tokenizer.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/TokenTagTable.h"
#include "Generic/tokens/SymbolList.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Metadata.h"

#include <wchar.h>

#include <iostream>
#include <boost/scoped_ptr.hpp>
using namespace std;


Token* UrduTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS+1];
Token* UrduTokenizer::_tempTokenBuffer[MAX_SENTENCE_TOKENS+1];
wchar_t UrduTokenizer::_char_buffer[MAX_TOKEN_SIZE+1];

static wchar_t PERIOD[] = {0x06d4, 0x002e,0};
static wchar_t COLON[] = {0x003a,0};
static wchar_t COMMA[] = {0x060c, 0x002c,0};
static wchar_t URL_PUNCTUATION[] = {0x005f, 0x0025, 0x003d, 0x002d, 0x0024, 0x002f,0}; // _%=-$/
static wchar_t NUMBER_PUNCTUATION[] = {0x002c, 0x060c, 0x0025, 0x0024,0}; //,،%$
static wchar_t REMAINING_PUNCTUATION[] = {0x2026, 0x061b, 0x007e, 0x005c, 0x2035, 0x01c0, 0x003b, 
										0x0021, 0x003f, 0x0060, 0x0027, 0x0022, 0x00b4, 0x007b, 0x007d,
										0x0028, 0x0029, 0x005b, 0x005d, 0x002d, 0x061f, 
										0xfe51, 0x2019, 0x2018, 0}; //…؛~\‵ǀ;!?`'"´{}()[]-؟﹑’‘
static wchar_t DIRECTIONAL_SYMBOLS[] = {0x202a, 0x202b, 0x202c};


/// this  is US dollar, cent, GB pound, currency, Yen, franc, afghani,
static wchar_t PREFIX_CURRENCY_SYMBOLS[] = 
	{L'$',0xa2,0xa3,0xa4,0xa5,0x192,0x60b,
///  bengali rupee, bengali rupee, gujarati rupee, tamil rupee, baht, riel,
	  0x9f2,0x9f3,0xaf1,0xbf9,0xe3f,0x17db,
///  script-big-m, cjk?, cjk?, cjk?, cjk?, rial
      0x2133,0x5143,0x5186,0x5706,0x5713,0xfdfc,0};
/// range is correct as of Unicode v5.1 2008
static wchar_t LOW_UNICODE_CURRENCY_INDEX = 0x20a0;
static wchar_t HIGH_UNICODE_CURRENCY_INDEX = 0x20b5;


// IMPORTANT NOTE:
// Keep in mind that the LocatedString class uses half-open
// intervals [start, end) while the Metadata and Span classes
// use closed intervals [start, end]. This is an easy source
// of off-by-one errors.

/**
* Urdu does not use punctuation to abbreviate or concatenate words as in English
* Therefore we only need to worry about not splitting for urls, emails, and numbers (decimals, large numbers, and time)
* Spacing tends to be inconsistent, therefore we need to add spaces on both sides of
* each punctuation in order to be sure that it will be tokenized.
* If this messes up tokenization on any English that happens to be there, ok.
*/
UrduTokenizer::UrduTokenizer() {
	
	_substitutionMap = NULL;
	std::string file_name = ParamReader::getRequiredParam("tokenizer_subst");
	_substitutionMap = _new SymbolSubstitutionMap(file_name.c_str());
}

UrduTokenizer::~UrduTokenizer() {
	if (_substitutionMap != NULL) {
		delete _substitutionMap;
	}
}

/**
 * @param doc the document being tokenized.
 * @param sent_no the index number of the next sentence that will be read.
 */
void UrduTokenizer::resetForNewSentence(const Document *doc, int sent_no){
	_document = doc;
	_cur_sent_no = sent_no;
	
}

/**
 * Puts an array of pointers to TokenSequences where specified by results arg,
 * and returns its size. Returns <code>0</code> if something goes wrong. The
 * caller is responsible for deleting the array and the TokenSequences.
 *
 * @param results the array of token sequences.
 * @param max_theories the maximum number of sentence theories to produce.
 * @param string the input string of the sentence.
 * @return the number of sentence theories produced; this will always be
 *         <code>1</code> on success and <code>0</code> on failure for this
 *         tokenizer.
 */
int UrduTokenizer::getTokenTheories(TokenSequence **results,
								int max_theories,
								const LocatedString *string, bool beginOfSentence,
								bool endOfSentence)
{
	// Make a local copy of the sentence string
	// MEM: make sure it gets deleted!
	
	LocatedString  *localString = _new LocatedString(*string);
	if (max_theories < 1)
		return 0;

	// use the local string to add spaces wherever we want to separate tokens
	// L"؟۔-…_"
	
	replaceDirectionalChar(localString);	
	splitPunctuationWithConstraints(localString, PERIOD, true, true);
	splitPunctuationWithConstraints(localString, COLON, true, true);
	splitPunctuationWithConstraints(localString, COMMA, false, true);
	splitPunctuationWithConstraints(localString, URL_PUNCTUATION, true, false);
	splitPunctuationWithConstraints(localString, NUMBER_PUNCTUATION, false, true);
	splitPunctuationWithConstraints(localString, REMAINING_PUNCTUATION, false, false);
	splitNumbersFromWords(localString);

	//	splitCurrencyAmounts(localString);
	// Now find the tokens, breaking at whitespace.
	int token_index = 0;
	int string_index = 0;
	while ((string_index < localString->length())) {
		if (token_index == MAX_SENTENCE_TOKENS) {
			SessionLogger::warn("too_many_tokens_15")
				<< "Sentence exceeds token limit of " << MAX_SENTENCE_TOKENS;
			break;
		}

		Token *nextToken = getNextToken(localString, &string_index);
		if (nextToken != NULL) {
			_tokenBuffer[token_index] = nextToken;
			token_index++;
		}
		string_index++;
	}


	results[0] = _new TokenSequence(_cur_sent_no, token_index, _tokenBuffer);

	delete localString;

	return 1;
}

void UrduTokenizer::splitNumbersFromWords(LocatedString *string) {
	wchar_t thischar;
	wchar_t nextchar;
	for (int i=0; i<string->length()-1; i++) {
		thischar = string->charAt(i);
		nextchar = string->charAt(i+1);
		if (iswdigit(thischar) && iswalpha(nextchar) ||
			iswalpha(thischar) && iswdigit(nextchar)) {
			string->insert(L" ",i+1);
			i++;
		}
	}
}
/**
* Place a space before and after any instance of the given characters,
* if the optional additional constraints regarding urls and digits are met
*/
void UrduTokenizer::splitPunctuationWithConstraints(LocatedString *string, const wchar_t punc[], bool checkURL, bool checkDigits) {
	wchar_t wch;
	bool replaceable;
	for (int i=0; i < string->length(); i++) {
		wch = string->charAt(i);
		replaceable = true;
		//if (isRelevantPunctuation(wch, punc)) {
		if (isRelevantPunctuation(wch, punc)) {
			if (checkURL && (matchURL(string, i) || matchEmail(string, i))) {
				replaceable = false;
			}
			if (checkDigits) {
				if (!isCommaBreakable(string, i)) {
					replaceable = false; 
				}
			}
			if (replaceable) {
				if (i < string->length()-1) {
					string->insert(L" ",i+1);
					string->insert(L" ",i);
					i = i+2;
				}
				else {
					string->insert(L" ",i);
					i++;
				}
			}
		}
	}
}

/*
* Return true if spaces should be put around the comma, which is
* always true at the ends of a sentence, and always in the middle unless
* the comma is surrounded on both sides by numbers
*/
bool UrduTokenizer::isCommaBreakable(LocatedString *string, int i) {
	if (i == 0 || i == string->length()-1) {
		return true;
	}
	else {
		wchar_t prevchar = string->charAt(i-1);
		wchar_t nextchar = string->charAt(i+1);
		if (iswdigit(prevchar) && iswdigit(nextchar))
			return false;
		else 
			return true;
	}
}

bool UrduTokenizer::isRelevantPunctuation(const wchar_t c, const wchar_t punc[]) const {
	return wcschr(punc, c) != NULL;
}


bool UrduTokenizer::matchEmail(LocatedString *string, int index) {
	int prev_whitespace = 0;
	int i = 0;
	for (i = index; i >= 0; i--) {
		if (iswspace(string->charAt(i))) {
			prev_whitespace = i;
			break;
		}
	}
	int at = string->indexOf(L"@", prev_whitespace);
	if (at > 0 && at < index)
		return true;
	return false;
}

/**
* Get rid of characters that encode left-to-right directionality
*/
void UrduTokenizer::replaceDirectionalChar(LocatedString *string) {
	for (int i=0; i<string->length(); i++) {
		wchar_t wch = string->charAt(i);
		if (wcschr(DIRECTIONAL_SYMBOLS, wch) != NULL) {
			string->replace(i,1,L" ");
			SessionLogger::info("tokenizer") << "used replaceDirectionalChar";
		}
	}
}

/**
 * @param string a pointer to the input text.
 * @param index on input, a pointer to the index into the string to start reading
 *              from; on output, a pointer to the next index into the string to
 *              start reading the next token from.
 * @return a pointer to the next token.
 */
Token* UrduTokenizer::getNextToken(const LocatedString *string, int *index) const {
	int start = *index;

	// Skip past any initial whitespace.
	while ((start < string->length()) &&
		   (iswspace(string->charAt(start))))
	{
		start++;
	}

	// If we're at the end of the string, there was no token.
	if (start == string->length()) {
		*index = start;
		return NULL;
	}

	// Search for the end of the token.
	int end = start + 1;
	while ((end < string->length()) &&
		   (!iswspace(string->charAt(end))) &&
		   (!enforceTokenBreak(string, end)))
	{
		end++;
	}
	if ((end - start) > MAX_TOKEN_SIZE) {
		SessionLogger::warn("token_too_big_9")
			<< "Number of characters in token exceeds limit of " << MAX_TOKEN_SIZE;
		end = start + MAX_TOKEN_SIZE;
	}

	// Copy the characters into the temp buffer and create a Symbol.
	int j = 0;
	for (int i = start; i < end; i++) {
		_char_buffer[j] = string->charAt(i);
		j++;
	}
	_char_buffer[j] = L'\0';
	Symbol sym = _substitutionMap->replace(Symbol(_char_buffer));

	// Set the index out-parameter to one beyond the end of the token.
	*index = end;
	// EDT-HACK
	return _new Token(string, start, end-1, sym);

}

/**
 * @param string the located string source of the sentence.
 * @param i the position at which to check for metadata.
 * @return <code>true</code> if the document metadata enforces a token break
 *         immediately before the given position; <code>false</code> if not.
 */
bool UrduTokenizer::enforceTokenBreak(const LocatedString *string, int i) const {
	Metadata *metadata = _document->getMetadata();
	Span *span;

	// Is there a span ending just before this position?
	span = metadata->getEndingSpan(string->end<EDTOffset>(i - 1));
	if (span != NULL &&
		// don't break in middle of replaced token
		// (replaced tokens causes several characters in the string to have the same offset)
		string->end<EDTOffset>(i - 1) != string->end<EDTOffset>(i) &&
		span->enforceTokenBreak())
	{
		return true;
	}

	// Is there a span starting right at this position?
	span = metadata->getStartingSpan(string->start<EDTOffset>(i));
	if (span != NULL &&
		// don't break in middle of replaced token
		// (replaced tokens causes several characters in the string to have the same offset)
		(i == 0 || string->end<EDTOffset>(i - 1) != string->end<EDTOffset>(i)) &&
		span->enforceTokenBreak()) {
		return true;
	}

	return false;
}


void UrduTokenizer::splitCurrencyAmounts(LocatedString *string) {
	// Search the string for dollar signs and other similar marks for popular
	// currencies.   We will only insert a space
	// if the following char is a digit, so we can stop at the
	// next-to-last character. This means peeking at the next char is
	// always safe.

	int i = 0;
	while (i < (string->length() - 1)) {
		wchar_t wch = string->charAt(i);
		if (iswdigit(string->charAt(i + 1)) &&
			(((wch >= LOW_UNICODE_CURRENCY_INDEX) &&
			  (wch <= HIGH_UNICODE_CURRENCY_INDEX))  ||
			 (wcschr(PREFIX_CURRENCY_SYMBOLS, wch) != NULL)))
		{
			SessionLogger::info("tokenizer") << "used splitCurrencyAmounts";
			string->insert(L" ", i + 1);
		}
		i++;
	}
}
