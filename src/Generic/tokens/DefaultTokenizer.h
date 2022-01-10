// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_TOKENIZER_H
#define DEFAULT_TOKENIZER_H

#include "Generic/common/limits.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/SymbolList.h"
#include <boost/regex_fwd.hpp>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DefaultTokenizer : public Tokenizer {

public:
	DefaultTokenizer();
	~DefaultTokenizer();
	void resetForNewSentence(const Document* doc, int sent_no);

	int getTokenTheories(TokenSequence **results, int max_theories,
						 const LocatedString *string, 
						 bool beginOfSentence = true,
						 bool endOfSentence =true);

private:
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	/// The internal buffer of characters used to construct individual tokens.
	static wchar_t _char_buffer[MAX_TOKEN_SIZE+1];

	Token* getNextToken(LocatedString* local_string, int *string_index) const;
	int _cur_sent_no;
	const Document* _document;

	std::set<wchar_t> _splitAfterChars;
	std::set<wchar_t> _splitBeforeChars;
	void fillWCharSet(std::string filename, std::set<wchar_t>& set_to_fill);
	void fillWordSet(std::string filename, std::set<std::wstring>& set_to_fill);

	std::set<std::wstring> _noSplitTokens;
	bool _do_split_chars;
	bool _check_no_split_tokens;
	std::vector<boost::wregex> _noSplitRegexs;
	bool _debug_no_split_regex;

	int findOtherSplitCharacter(LocatedString* string, int start, int end) const;
	bool isTokenSplittable(LocatedString *string, int start, int end) const;

	// The token substitution map.
	SymbolSubstitutionMap *_substitutionMap;
};

#endif
