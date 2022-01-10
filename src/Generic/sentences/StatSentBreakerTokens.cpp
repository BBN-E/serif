// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/sentences/StatSentBreakerTokens.h"


#define TOKEN_SPLIT_PUNCTUATION        L".[](){},\"'`?!:;$"

SymbolSubstitutionMap *StatSentBreakerTokens::_substitutionMap = 0;
Symbol StatSentBreakerTokens::HARD_NEW_LINE = Symbol(L"-HNL-");

StatSentBreakerTokens::StatSentBreakerTokens() : _maxLength(MAX_REGION_TOKENS) {
	_tokens = _new Symbol[_maxLength];
	_token_starts = _new int[_maxLength];
	_token_ends = _new int[_maxLength];
	_length = 0;
}

StatSentBreakerTokens::~StatSentBreakerTokens() {
	delete [] _tokens;
	delete [] _token_starts;
	delete [] _token_ends;
}

void StatSentBreakerTokens::initSubstitutionMap(const char *model_file) {
	_substitutionMap = _new SymbolSubstitutionMap(model_file);
}

/** 
* Initializes tokens from an existing LocatedString
*
*/
void StatSentBreakerTokens::init(const LocatedString *str) {
	// Reset length to 0
	_length = 0;

	// Make a local copy of the sentence string
	LocatedString  *localString = _new LocatedString(*str);

	int string_index = 0;
	while ((string_index < localString->length())) {
		if (_length == _maxLength) {
			SessionLogger::warn("too_many_tokens") << "Sentence exceeds token limit of "
				   << _maxLength << "\n";
			break;
		}

		bool found = getNextToken(localString, &string_index);
		if (!found) 
			break;
	}

	delete localString;
}

/** 
* Reads training sentence from filestream (one per line)
* 
* @return true if successful in reading new sentence, false if EOF or other error
*/
bool StatSentBreakerTokens::readTrainingSentence(UTF8InputStream& stream) 
throw(UnexpectedInputException) {
	
	LocatedString str(stream, L'\n');
	
	if (str.length() == 0) {
		_length = 0;
		return false;
	}
	
	init(&str);
	return _length > 0;
}

StatSentBreakerTokens &StatSentBreakerTokens::operator=(const StatSentBreakerTokens &other) {
	if (other._length <= _maxLength)
		_length = other._length;
	else
		_length = _maxLength;

	for (int i = 0; i < _length; i++) {
		_tokens[i] = other._tokens[i];
		_token_starts[i] = other._token_starts[i];
		_token_ends[i] = other._token_ends[i];
	}

	return *this;
}

// separate ., .'', . ..., . '', . ... from previous word
// separate ), (, ?, !, :,  ;, '', ``, ... from non-space before and after
// separate $ from anything after
// separate ' from word characters before, but not after
// insert HARD_NEW_LINE in place of double newlines
// TODO: need to deal with double quotes and non-English languages
bool StatSentBreakerTokens::getNextToken(const LocatedString *string, int *index) {
	int start = *index;

	// Skip past any initial whitespace, unless there are double newlines.
	while ((start < string->length()) &&
		   (iswspace(string->charAt(start))))
	{
		if (string->charAt(start) == L'\n') {
			int temp = start + 1;
			while ((temp < string->length()) &&
				   (iswspace(string->charAt(temp)) &&
				   (string->charAt(temp) != L'\n')))
			{
				temp++;
			}
			if ((temp < string->length()) &&
				(string->charAt(temp) == L'\n')) 
			{
				// Set the index parameter to one beyond the end of the token.
				*index = temp + 1;
				_tokens[_length] = HARD_NEW_LINE;
				_token_starts[_length] = start;
				_token_ends[_length] = temp + 1;
				_length++;
				return true;
			}
		}
		start++;
	}

	// If we're at the end of the string, there was no token.
	if (start == string->length()) {
		*index = start;
		return false;
	}

	
	// Search for the end of the token.
	int end = start + 1;
	if (!isSplitChar(string->charAt(start))) {
		while ((end < string->length()) &&
			  (!iswspace(string->charAt(end))) &&
			  (!isSplitChar(string->charAt(end))))
		{
			end++;
		}
		// if it's a period and there's no space or punct following, keep going
		while ((end + 1 < string->length()) && 
			   (string->charAt(end) == '.') &&
			   (!iswspace(string->charAt(end+1))) &&
			   (!isSplitChar(string->charAt(end+1)))) 
		{
			end++;
			while ((end < string->length()) &&
				  (!iswspace(string->charAt(end))) &&
				  (!isSplitChar(string->charAt(end))))
			{
				end++;
			}
		}
		// if it's a comma and there's no space following, keep going
		while ((end + 1 < string->length()) && 
			   (string->charAt(end) == ',') &&
			   (!iswspace(string->charAt(end+1)))) 
		{
			end++;
			while ((end < string->length()) &&
				  (!iswspace(string->charAt(end))) &&
				  (!iswpunct(string->charAt(end))))
			{
				end++;
			}
		}
	} else {
		if (string->charAt(start) == '.' && 
			start + 3 < string->length() &&
			string->charAt(start+1) == '.' &&
			string->charAt(start+2) == '.' &&
			string->charAt(start+3) == '.') 
		{
			; // do nothing - we want to end up with ". ..."
		}
		else if (string->charAt(start) == '.') {
			while ((end < string->length()) && string->charAt(end) == '.')
				end++;
		}
		else if (string->charAt(start) == '`') {
			if (end < string->length() && string->charAt(end) == '`') {
				end++;
			}
		}
		else if (string->charAt(start) == '\'') {
			if (end < string->length() && string->charAt(end) == '\'') {
				end++;
			}
			else {
				while ((end < string->length()) &&
					(!iswspace(string->charAt(end))) &&
					(!isSplitChar(string->charAt(end))))
				{
					end++;
				}
			}
		}
	}

	if ((end - start) > MAX_TOKEN_SIZE) {
		SessionLogger::warn("token_too_big")
			<< "Number of characters in token exceeds limit of " << MAX_TOKEN_SIZE;
		end = start + MAX_TOKEN_SIZE;
	}

	// Copy the characters into the temp buffer and create a Symbol.
	int j = 0;
	wchar_t char_buffer[MAX_TOKEN_SIZE + 1];
	for (int i = start; i < end; i++) {
		char_buffer[j] = string->charAt(i);
		j++;
	}
	char_buffer[j] = L'\0';
	Symbol sym = _substitutionMap->replace(Symbol(char_buffer));

	// Set the index parameter to one beyond the end of the token.
	*index = end;

	_tokens[_length] = sym;
	_token_starts[_length] = start;
	_token_ends[_length] = end;
	_length++;
	
	return true;
}

bool StatSentBreakerTokens::isSplitChar(wchar_t c) const {
	// TODO: could make this more efficient if it mattered
	return wcschr(TOKEN_SPLIT_PUNCTUATION, c) != NULL;
}
