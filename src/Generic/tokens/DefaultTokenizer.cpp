// Copyright 2011 BBN Technologies
// All Rights Reserved

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InputUtil.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/DefaultTokenizer.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/SymbolList.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
using namespace std;
class Token;
class Tokenizer;
class DefaultTokenizer;

Token* DefaultTokenizer::_tokenBuffer[MAX_SENTENCE_TOKENS+1];
wchar_t DefaultTokenizer::_char_buffer[MAX_TOKEN_SIZE+1];

/*
	* Always split on white space
	* If a set of e.g. punctuation characters are given as
	* split characters, separate those as well
	*  (you can specify characters that split only before, only after, or both)
	* If a set of non-split tokens are given as counter-examples
	* to the split characters, don't split them
*/
DefaultTokenizer::DefaultTokenizer(): _substitutionMap(NULL)
{
	_do_split_chars = false;
	_check_no_split_tokens = false;

	std::set<wchar_t> splitChars;
	fillWCharSet(ParamReader::getParam("tokenizer_split_chars"), splitChars);
	fillWCharSet(ParamReader::getParam("tokenizer_split_after_chars"), _splitAfterChars);
	fillWCharSet(ParamReader::getParam("tokenizer_split_before_chars"), _splitBeforeChars);
	BOOST_FOREACH(wchar_t c, splitChars) {
		_splitAfterChars.insert(c);
		_splitBeforeChars.insert(c);
	}

	_do_split_chars = _splitAfterChars.size() != 0 || _splitBeforeChars.size() != 0;

	fillWordSet(ParamReader::getParam("tokenizer_no_split_tokens"), _noSplitTokens);
	fillWordSet(ParamReader::getParam("tokenizer_non_final_abbrevs"), _noSplitTokens);

	//_noSplitRegexs;
	std::string noSplitRegexsFilename = ParamReader::getParam("tokenizer_no_split_regexs");
	_debug_no_split_regex = ParamReader::isParamTrue("debug_tokenizer_no_split_regexs");
	if (!noSplitRegexsFilename.empty()) {
		std::set<std::wstring> lines = InputUtil::readFileIntoSet(noSplitRegexsFilename, true, false);
		BOOST_FOREACH(std::wstring line, lines) {
			_noSplitRegexs.push_back(boost::wregex(line));
		}
	}

	std::string tokenizer_subst = ParamReader::getParam("tokenizer_subst");
	if (!tokenizer_subst.empty())
		_substitutionMap = _new SymbolSubstitutionMap(tokenizer_subst.c_str());
}

DefaultTokenizer::~DefaultTokenizer() {
	delete _substitutionMap;
}

void DefaultTokenizer::fillWCharSet(std::string filename, std::set<wchar_t>& set_to_fill) {
	if (filename == "")
		return;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filename.c_str());

	std::wstring line;
	while (!in.eof()) {
		in.getLine(line);
		boost::algorithm::trim(line);
		if (line.size() > 0) {
			set_to_fill.insert(line.at(0));
		}
	}
	in.close();
}

// Adds lowercase versions as well
void DefaultTokenizer::fillWordSet(std::string filename, std::set<std::wstring>& set_to_fill) {
	if (filename == "")
		return;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filename.c_str());

	std::wstring line;
	while (!in.eof()) {
		std::getline(in, line);
		boost::algorithm::trim(line);
		if (line.size() > 0 && line.at(0) != L'#') {
			set_to_fill.insert(line);
			std::transform(line.begin(), line.end(), line.begin(), towlower);
			set_to_fill.insert(line);
		}
	}
	in.close();
}

void DefaultTokenizer::resetForNewSentence(const Document *doc, int sent_no)
{
	_document = doc;
	_cur_sent_no = sent_no;
}

int DefaultTokenizer::getTokenTheories(TokenSequence **results, int max_theories,
		const LocatedString *string, bool beginOfSentence, bool endOfSentence)
{
	// Make a local copy of the sentence string
	boost::scoped_ptr<LocatedString> localString(_new LocatedString(*string));
	if (max_theories < 1)
		return 0;

	// Normalize unicode characters.
	removeHarmfulUnicodes(localString.get());
	replaceNonstandardUnicodes(localString.get());

	// Find the tokens, breaking at whitespace.
	int token_index = 0;
	int string_index = 0;
	while ((string_index < localString->length())) {
		if (token_index == MAX_SENTENCE_TOKENS) {
			std::wstringstream errMsg;
			errMsg << L"Sentence exceeds token limit of " << MAX_SENTENCE_TOKENS << L". The remainder of the sentence will be discarded.\n";
			errMsg << L"Full sentence: " << localString->toWString() << L"\n";
			errMsg << L"Truncated sentence: " << localString->substringAsWString(0, string_index) << L"\n";
			SessionLogger::warn_user("too_many_tokens") << errMsg.str().c_str();				
			break;
		}
	
		Token *nextToken = getNextToken(localString.get(), &string_index);
		if (nextToken != NULL) 
			_tokenBuffer[token_index++] = nextToken;
		
	}

	if (token_index == 0) {		
		// use original string here, so we get reasonable EDT offsets!!
		int length = string->length();
		_tokenBuffer[token_index++] = _new Token(string, 0, length-1, Symbol(L"-empty-"));
	}

	results[0] = _new TokenSequence(_cur_sent_no, token_index, _tokenBuffer);
	
	return 1;
}


Token* DefaultTokenizer::getNextToken(LocatedString* string, int *index) const
{
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

	// Search for the end of the token using whitespace
	int end = start + 1;
	while ((end < string->length()) &&
		   (!iswspace(string->charAt(end))))
	{
		end++;
	}
	if ((end - start) > MAX_TOKEN_SIZE) {
		SessionLogger::warn("token_too_big")
			<< "Number of characters in token exceeds limit of " << MAX_TOKEN_SIZE;
		end = start + MAX_TOKEN_SIZE;
	}

	if (_do_split_chars &&
		isTokenSplittable(string, start, end))
			end = findOtherSplitCharacter(string, start, end);
		
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

int DefaultTokenizer::findOtherSplitCharacter(LocatedString* string, int start, int end) const
{
	boost::scoped_ptr<std::set<int> > noSplitBefore;
	if (!_noSplitRegexs.empty()) {
		noSplitBefore.reset(new std::set<int>());
		std::wstring wstr = string->toWString();
		BOOST_FOREACH(boost::wregex regex, _noSplitRegexs) {
			boost::wsregex_iterator iter(wstr.begin()+start, wstr.begin()+end, regex);
		    boost::wsregex_iterator end;
			for( ; iter != end; ++iter ) {
				int matchStart = static_cast<int>(start+iter->position());
				int matchEnd = static_cast<int>(matchStart+iter->length());
				for (int i=matchStart+1; i<matchEnd; ++i)
					noSplitBefore->insert(i);
			}
		}
	}

	// if the first char is a split char, 
	// step forward until a non-split char is found
	for (int j=start; j < end; ++j) {
		wchar_t c = string->charAt(j);
		if (j != start && _splitBeforeChars.find(c) != _splitBeforeChars.end()) {
			// If a no-split regex matched here, then we shouldn't split.
			if (!noSplitBefore || (noSplitBefore->find(j)==noSplitBefore->end())) {
				// this is the last character and it's a split-before, then SPLIT
				// otherwise, only do so if we are NOT a period or colon, since these often
				// signify email/web addresses
				if (j == end-1)
					return j;
				else if (c != L'.' && c != L':')
					return j;
			} else if (_debug_no_split_regex) {
				std::cerr << "  Blocked Before-Split: "
					<< string->substringAsWString(start, j) << " "
					<< string->substringAsWString(j, end) << std::endl;
			}
		}
		if (j < end + 1 && _splitAfterChars.find(c) != _splitAfterChars.end()) {
			// same deal as above
			if (!noSplitBefore || (noSplitBefore->find(j+1)==noSplitBefore->end())) {
				if (j == end-1)
					return j+1;
				else if (c != L'.' && c != L':')
					return j+1;
			} else if (_debug_no_split_regex) {
				std::cerr << "  Blocked After-Split: "
					<< string->substringAsWString(start, j+1) << " "
					<< string->substringAsWString(j+1, end) << std::endl;
			}
		}
	}
	
	// no split characters found
	return end;
}

bool DefaultTokenizer::isTokenSplittable(LocatedString *string, int start, int end) const
{
	// return true if the token is **not** in the list of no-split tokens
	std::wstring tokStr = string->substringAsWString(start, end);
	return (_noSplitTokens.find(tokStr) == _noSplitTokens.end());	
}
