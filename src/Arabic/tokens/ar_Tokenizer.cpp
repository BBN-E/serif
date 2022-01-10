// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/tokens/ar_Tokenizer.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/LexicalToken.h"
#include "Generic/theories/LexicalTokenSequence.h"

#include <wchar.h>

#include <iostream>
using namespace std;

Token* ArabicTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS+1];
wchar_t ArabicTokenizer::_char_buffer[MAX_TOKEN_SIZE+1];
/*This produces a preliminary Tokenization-
* The parser is responsible for clitic Tokenization
*/

/*
* a. sentence end	->	a_.		:sentence final .
* a.a sentence mid.	->	a_._a	:(periods are never parts of words?)
* ...				->	._._.	:training treats ... as ._._.
* d.d				->	d.d		:numbers, taken care of by number separator
*/

#define SPLIT_PUNCTUATION        L"[](),\"`?!:;.'{}%/\\+-" //don't add hyphen for now b/c -lrb-,-rbr-, etc
													 //and no hyphenated names
													//remove ' b/c it is used phonetically in transliteration
													///remove {} for same reason
													//READD in Arabic Text!!!!
													//% is added to match TB tokenization


ArabicTokenizer::ArabicTokenizer() {
	_substitutionMap = NULL;
	_use_itea_tokenization = false;
	_use_GALE_tokenization = false;

	std::string file_name = ParamReader::getRequiredParam("tokenizer_subst");
	_substitutionMap = _new SymbolSubstitutionMap(file_name.c_str());
	_use_GALE_tokenization = ParamReader::isParamTrue("use_GALE_tokenization");

}
ArabicTokenizer::~ArabicTokenizer() {
	if (_substitutionMap != NULL) {
		delete _substitutionMap;
	}
}

void ArabicTokenizer::removeLinesWithoutArabicCharacters(LocatedString* string) {
	int start = 0;	
	int end = string->indexOf(L"\n", start+1);
	while (start >= 0 && end >= 0) {
		bool foundArabic = false;

		// Check if we have a very long line
		// If so, require it to have at least 20% Arabic
		// Otherwise, a single Arabic character is good enough
		int length = end - start;
		if (length > 10000) {
			int count = 0;
			for (int i = start; i < end; i++) {
				wchar_t wch = string->charAt(i);
				if ((0x0600 < wch) && (wch < 0x06ff)) {
					count++;
				}	
			}
			double percentage = (count * 1.0) / length;
			if (percentage > .2) { 
				foundArabic = true;
			}
		} else {
			for (int i = start; i < end; i++) {
				wchar_t wch = string->charAt(i);
				if ((0x0600 < wch) && (wch < 0x06ff)) {
					foundArabic = true;
					break;
				}	
			}
		}
		if (foundArabic) {
			start = end+1;
		} else {
			string->remove(start, end+1); // end argument is exclusive
			// Keep start the same since it will now point to the start of a new line
		} 
		end = string->indexOf(L"\n", start+1);			
	}
}

void ArabicTokenizer::resetForNewSentence(const Document *doc, int sent_no) {
	_cur_sent_no = sent_no;
}

/**
 * Puts an array of pointers to TokenSEquences where specified by results arg,
 * and returns its size. Returns <code>0</code> if something goes wrong. The
 * caller is responsible for deleting the array and the TokenSequences.
 */
int ArabicTokenizer::getTokenTheories(TokenSequence **results,
								int max_theories,
								const LocatedString *string,bool beginOfSentence,
								 bool endOfSentence)
{
	// Make a local copy of the sentence string
	// MEM: make sure it gets deleted!
	LocatedString  *localString = _new LocatedString(*string);

	if (max_theories < 1)
		return 0;

	if (!beginOfSentence || !endOfSentence) {
		SessionLogger::warn("incomplete_sentence")
			<< "ArabicTokenizer::getTokenTheories(), " 
			<< "ArabicTokenizer does not know how to handle incomplete sentences.\n";
	}
	// These use the generic code, so  specialize it if needed		
	int subs = removeHarmfulUnicodes(localString);
	if (subs > 0) {
		SessionLogger::warn("bad_unicode") << "removed harmful Unicode " << subs << " chars in tokenizer\n";
	}
	subs = replaceNonstandardUnicodes(localString);
	if (subs > 0) {
		SessionLogger::warn("bad_unicode") << "replaced Non-standard Unicode " << subs << " chars in tokenizer\n";
	}

	replaceExcessivePunctuation(localString);

	// Perform string replacements:
	eliminateDoubleQuotes(localString);
	
	if(_use_GALE_tokenization){
		replaceHyphenGroups(localString);
	}
	
	normalizeWhitespace(localString);

	// Now find the tokens, breaking at whitespace.
	int token_index = 0;
	int string_index = 0;
	while ((string_index < localString->length())) {
		if (token_index == MAX_SENTENCE_TOKENS-1) {
			SessionLogger::warn("too_many_tokens")
				<< "Sentence Exceeds token limit of " << MAX_SENTENCE_TOKENS;
			break;
		}

		Token *nextToken = getNextToken(localString, &string_index);
		//debugging
		//logger.beginMessage();
		//logger<<"token "<<token_index<<": "<<nextToken->getSymbol().to_string()<<"\n";
		if (nextToken != NULL) {
			_tokenBuffer[token_index] = nextToken;
			token_index++;
		}
	}
	//logger<<"make token sequence\n";

	results[0] = _new LexicalTokenSequence(_cur_sent_no, token_index, _tokenBuffer);
//	logger<<"done with getTokenTheories\n";

	delete localString;

	return 1;
}

Token* ArabicTokenizer::getNextToken(const LocatedString *string, int *index) const {
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
	int end = start;
	while ((end < string->length()) &&
		   (!iswspace(string->charAt(end))))
	{
		end++;
	}
	if ((end - start) > MAX_TOKEN_SIZE) {
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
	return _new LexicalToken(string, start, end-1, sym);

}

// +---------------+----------------+
// |  Don't split  |	Split both	|
// +---------------+----------------+
// |  1,000.65     |	_,_			|
// |		       |	_!_			|
// |               |	_:_			|
// |			   |	_._			|
// +---------------+----------------+
//Differences between Arabic and English
//In Arabic-
//	1) Always separate non numeric '.' - see note at top of file about periods
//	2) Always separate single ' (it is never a contraction)
//	TODO: Fix this?
//	3) Arabic training only has open quotes, so for now '''->''_' always

void ArabicTokenizer::normalizeWhitespace(LocatedString *string) {

	// NOTE: We only ever modify the string *after* the current position.
	for (int i = 0; i < string->length() - 1; i++) {
		wchar_t curr_char = string->charAt(i);

		// Collapse whitespace.
		if (iswspace(curr_char)) {
			int end = skipWhitespace(string, i);
			if (end > (i + 1)) {
				string->remove(i + 1, end);
			}
		}
		// The current character is a punctuation mark, so we
		// may need to insert a space after it.
		//'''a	-> ''_'_a TODO: Add open quotes!
		//''a	-> ''_a
		//Pa	-> P_a
		else if (isSplitChar(curr_char) &&
				!matchURL(string, i) &&
				 !matchNumberSeparator(string, i) &&
				 !iswspace(string->charAt(i + 1)))
		{
			if (matchCloseQuotes(string, i) == 3 &&
				isSplittable(string, i + 3) &&
				isSplittable(string, i + 1)) 
			{
				string->insert(L" ", i + 3);
				string->insert(L" ", i + 1);
				i += 4;
			}
			else if (matchCloseQuotes(string, i) == 2 &&
				isSplittable(string, i + 2))
			{
				string->insert(L" ", i + 2);
				i += 2;
			}
			else if (isSplittable(string, i + 1)) {
				string->insert(L" ", i + 1);
				i++;
			}
		}

		// The next character is a punctuation mark, so we
		// may need to insert a space before and/or after it.
		else if (isSplitChar(string->charAt(i + 1)) &&
				!matchURL(string, i) && 
				 !matchNumberSeparator(string, i + 1) &&
				 !iswspace(string->charAt(i)))
		{
			// Three quotes get separated into a pair and a single one,
			// and preceded by a spaces. Don't advance beyond the last
			// quote mark, so in the next iteration, another space will
			// be inserted after.
			//TODO: Add open quotes
			if (matchCloseQuotes(string, i + 1) == 3 &&
				isSplittable(string, i + 2) &&
				isSplittable(string, i + 1)) 
			{
				string->insert(L" ", i + 2);
				string->insert(L" ", i + 1);
				i += 4;
			}
			// Two quotes stay together but get preceded by a space.
			// Don't advance beyond the last quote mark, so in the next
			// iteration, another space will be inserted after.
			else if (matchCloseQuotes(string, i + 1) == 2 &&
				isSplittable(string, i + 1))
			{
				string->insert(L" ", i + 1);
				i += 2;
			}
			// Any other kind of punctuation gets preceded by a space.
			// Don't advance beyond the quote mark, so in the next
			// iteration, another space will be inserted.
			else if (isSplittable(string, i + 1)) {
				string->insert(L" ", i + 1);
				i++;
			}
		}
	}
}
//TODO:  don't split sentences on number separators
bool ArabicTokenizer::matchNumberSeparator(const LocatedString *string, int index) const {
	//66B is Arabic Thousand Separator, 66C is Arabic decimal point-
	//660-669 are Arabic digits
	//TODO check that Arabic comma is not used as a number separator
	return (index > 0) &&
		   ((index + 1) < string->length()) &&
		   ((string->charAt(index) == L',')||(string->charAt(index) ==L'.')||
		   (string->charAt(index)==0x066B)||(string->charAt(index)==0x066C))
		   &&((iswdigit(string->charAt(index - 1))
			||((string->charAt(index - 1)>=0x0660) && (string->charAt(index - 1)<=0x0669))))
		   && ((iswdigit(string->charAt(index + 1))
			||((string->charAt(index + 1)>=0x0660) && (string->charAt(index + 1)<=0x0669))));
}




bool ArabicTokenizer::isSplitChar(wchar_t c) const {
	// TODO: could make this more efficient if it mattered
	//ARABIC SPLIT PUNCTUATION-
	//'%' 0x066A; 'star' 0x066D; ',' 0x060C; ';' 0x061B; '?' 0x061F;
	//0xAB '<<'; 0xBB '>>'; 0x00AD 'Soft-Hyphen'
	bool ret= wcschr(SPLIT_PUNCTUATION, c) != NULL;
	ret = ret || (c==0x066A) ||(c==0x066D)||(c==0x060C)||(c==0x061B)||(c==0x061f)
		|| (c==0x00AB) || (c==0x00BB) ||(c == 0x00AD);
	return ret;
}

/*
** Turn all " into '' - This is how the parser trained
*/
void ArabicTokenizer::eliminateDoubleQuotes(LocatedString *string) {
	for (int i = 0; i < string->length(); i++) {
		if (string->charAt(i) == L'"') {
			string->replace(i, 1, L"''");
		}
	}
}





