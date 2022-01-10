// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "tokens/KoreanTokenizer.h"
#include "common/LocatedString.h"
#include "common/limits.h"
#include "common/ParamReader.h"
#include "common/UTF8InputStream.h"
#include "common/Symbol.h"
#include "tokens/SymbolSubstitutionMap.h"
#include "common/SessionLogger.h"
#include "theories/Document.h"

#include <wchar.h>

#include <iostream>
using namespace std;

Token* KoreanTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS+1];
wchar_t KoreanTokenizer::_char_buffer[MAX_TOKEN_SIZE+1];
SymbolSubstitutionMap* KoreanTokenizer::_substitutionMap = 0;

/*This produces a preliminary Tokenization*/

/*
* a. sentence end	->	a_.		:sentence final .
* a.a sentence mid.	->	a_._a	:(periods are never parts of words?)
* ...				->	...		:training treats ... as ...
* d.d				->	d.d		:numbers, taken care of by number separator
*/

#define SPLIT_PUNCTUATION        L"[](),\"\'`?!:;.'{}%/\\+-" 					
												

KoreanTokenizer::KoreanTokenizer() {
	_substitutionMap = NULL;

	char file_name[501];
	if (!ParamReader::getParam("tokenizer_subst",file_name, 500))	{
		throw UnexpectedInputException("KoreanTokenizer::KoreanTokenizer()",
									   "Param `tokenizer_subst' not defined");
	}
	_substitutionMap = _new SymbolSubstitutionMap(file_name);

}

KoreanTokenizer::~KoreanTokenizer() {
	if (_substitutionMap != NULL) {
		delete _substitutionMap;
	}
}

void KoreanTokenizer::resetForNewSentenceRaw(Document *doc, int sent_no) {
	_cur_sent_no = sent_no;
}

/**
 * Puts an array of pointers to TokenSEquences where specified by results arg,
 * and returns its size. Returns <code>0</code> if something goes wrong. The
 * caller is responsible for deleting the array and the TokenSequences.
 */
int KoreanTokenizer::getTokenTheoriesRaw(TokenSequence **results,
								int max_theories,
								const LocatedString *string,bool beginOfSentence,
								 bool endOfSentence)
{
	// Get default session logger
	SessionLogger &logger = *SessionLogger::logger;

	// Make a local copy of the sentence string
	// MEM: make sure it gets deleted!
	LocatedString  *localString = _new LocatedString(*string);

	if (max_theories < 1)
		return 0;

	if (!beginOfSentence || !endOfSentence) {
		logger.beginWarning();
		logger << "KoreanTokenizer::getTokenTheoriesRaw(), " 
			   << "KoreanTokenizer does not know how to handle incomplete sentences.\n";
	}

	// Perform string replacements:
	//eliminateDoubleQuotes(localString);  // Double quotes not replaced for Korean
	normalizeWhitespace(localString);

	// Now find the tokens, breaking at whitespace.
	int token_index = 0;
	int string_index = 0;
	while ((string_index < localString->length())) {
		if (token_index == MAX_SENTENCE_TOKENS) {
			logger.beginWarning();
			logger << "Sentence Exceeds token limit of "
				   << MAX_SENTENCE_TOKENS << "\n";
			break;
		}

		Token *nextToken = getNextToken(localString, &string_index);
		if (nextToken != NULL) {
			_tokenBuffer[token_index] = nextToken;
			token_index++;
		}
	}

	results[0] = _new TokenSequence(_cur_sent_no, token_index, _tokenBuffer);

	delete localString;

	return 1;
}

Token* KoreanTokenizer::getNextToken(const LocatedString *string, int *index) const {
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
	// EDT-HACK
//	return new Token(string->offsetAt(start), string->offsetAt(end - 1), sym);
	return _new Token(string->edtBeginOffsetAt(start), string->edtEndOffsetAt(end - 1), sym);
}

// +---------------+----------------+
// |  Don't split  |	Split both	|
// +---------------+----------------+
// |  1,000.65     |	_,_			|
// |		       |	_!_			|
// |               |	_:_			|
// |			   |	_._			|
// +---------------+----------------+
//Differences between Korean and English
//In Korean-
//	1) Always separate non-numeric, non-grouped '.' - see note at top of file about periods
//	2) Always separate single ' (it is never a contraction)


void KoreanTokenizer::normalizeWhitespace(LocatedString *string) {
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
				 !matchNumberSeparator(string, i) &&
				 !matchMultiCharPunct(string, i) &&
				 !iswspace(string->charAt(i + 1)))
		{
			if (matchThreeCloseQuotes(string, i)) {
				string->insert(L" ", 1, i + 3);
				string->insert(L" ", 1, i + 1);
				i += 4;
			}
			else if (matchTwoCloseQuotes(string, i))
			{
				string->insert(L" ", 1, i + 2);
				i += 2;
			}
			else {
				string->insert(L" ", 1, i + 1);
				i++;
			}
		}

		// The next character is a punctuation mark, so we
		// may need to insert a space before and/or after it.
		else if (isSplitChar(string->charAt(i + 1)) &&
				 !matchNumberSeparator(string, i + 1) &&
				 !matchMultiCharPunct(string, i) &&
				 !iswspace(string->charAt(i)))
		{
			// Three quotes get separated into a pair and a single one,
			// and preceded by a spaces. Don't advance beyond the last
			// quote mark, so in the next iteration, another space will
			// be inserted after.
			//TODO: Add open quotes
			if (matchThreeCloseQuotes(string, i + 1)) {
				string->insert(L" ", 1, i + 2);
				string->insert(L" ", 1, i + 1);
				i += 4;
			}
			// Two quotes stay together but get preceded by a space.
			// Don't advance beyond the last quote mark, so in the next
			// iteration, another space will be inserted after.
			else if (matchTwoCloseQuotes(string, i + 1))
			{
				string->insert(L" ", 1, i + 1);
				i += 2;
			}
			// Any other kind of punctuation gets preceded by a space.
			// Don't advance beyond the quote mark, so in the next
			// iteration, another space will be inserted.
			else {
				string->insert(L" ", 1, i + 1);
				i++;
			}
		}
	}
}

bool KoreanTokenizer::matchNumberSeparator(const LocatedString *string, int index) const {
	return (index > 0) 
		   && ((index + 1) < string->length()) 
		   && ((string->charAt(index) == L',') || (string->charAt(index) == L'.') ||
		       (string->charAt(index) == L'-') || (string->charAt(index) == L':') ||
			   (string->charAt(index) == L'/'))
		   && iswdigit(string->charAt(index - 1))
		   && iswdigit(string->charAt(index + 1));
}

bool KoreanTokenizer::matchMultiCharPunct(const LocatedString *string, int index) const {
	return (index + 1 < string->length()) && 
		     (((string->charAt(index) == L'.') && (string->charAt(index + 1) == L'.'))
		   || ((string->charAt(index) == L'-') && (string->charAt(index + 1) == L'-'))
		   || ((string->charAt(index) == L'?') && (string->charAt(index + 1) == L'!'))
		   || ((string->charAt(index) == L'!') && (string->charAt(index + 1) == L'!')));	 
}

bool KoreanTokenizer::matchTwoOpenQuotes(const LocatedString *string, int index) const {
	return ((index + 1) < string->length()) &&
		   (string->charAt(index) == L'`') &&
		   (string->charAt(index + 1) == L'`');
}

bool KoreanTokenizer::matchTwoCloseQuotes(const LocatedString *string, int index) const {
	return ((index + 1) < string->length()) &&
		   (string->charAt(index) == L'\'') &&
		   (string->charAt(index + 1) == L'\'');
}

bool KoreanTokenizer::matchThreeOpenQuotes(const LocatedString *string, int index) const {
	return ((index + 2) < string->length()) &&
		   (string->charAt(index) == L'`') &&
		   (string->charAt(index + 1) == L'`') &&
		   (string->charAt(index + 2) == L'`');
}

bool KoreanTokenizer::matchThreeCloseQuotes(const LocatedString *string, int index) const {
	return ((index + 2) < string->length()) &&
		   (string->charAt(index) == L'\'') &&
		   (string->charAt(index + 1) == L'\'') &&
		   (string->charAt(index + 2) == L'\'');
}



bool KoreanTokenizer::isSplitChar(wchar_t c) const {
	return wcschr(SPLIT_PUNCTUATION, c) != NULL;
}

int KoreanTokenizer::skipWhitespace(const LocatedString *string, int index) const {
	int length = string->length();
	while ((index < length) && (iswspace(string->charAt(index))))
		index++;
	return index;
}

/*
** Turn all " into '' - This is how the parser trained
*/
void KoreanTokenizer::eliminateDoubleQuotes(LocatedString *string) {
	for (int i = 0; i < string->length(); i++) {
		if (string->charAt(i) == L'"') {
			string->replace(i, 1, L"''", 2);
		}
	}
}

Symbol KoreanTokenizer::getSubstitutionSymbol(Symbol sym) {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		char token_subst_file[501];
		ParamReader::getRequiredParam("tokenizer_subst", token_subst_file, 500);

		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file);
	}

	return _substitutionMap->replace(sym);
}
