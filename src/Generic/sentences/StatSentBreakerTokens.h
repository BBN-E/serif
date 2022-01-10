// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_BREAKER_STRING_H
#define STAT_SENT_BREAKER_STRING_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include <wchar.h>

class UTF8InputStream;
class LocatedString;
class SymbolSubstitutionMap;

/**
 * Stores the tokens for a given string. Used in training and decode for consistency.
 */
class StatSentBreakerTokens {
protected:
	int _length;
	int _maxLength;
	Symbol *_tokens;
	int *_token_starts;
	int *_token_ends;

	static Symbol HARD_NEW_LINE;
	static SymbolSubstitutionMap *_substitutionMap;

public:
	StatSentBreakerTokens();
	~StatSentBreakerTokens();

	StatSentBreakerTokens &operator=(const StatSentBreakerTokens &other);

	static void initSubstitutionMap(const char *model_file);

	void init(const LocatedString *str);
	bool readTrainingSentence(UTF8InputStream& stream) throw(UnexpectedInputException);
	
	Symbol getWord(int index) { return _tokens[index]; }
	void setWord(int index, Symbol word) { _tokens[index] =  word; }

	int getTokenStart(int index) { return _token_starts[index]; }
	int getTokenEnd(int index) { return _token_ends[index]; }

	int getLength() { return _length; }
	
private:
	bool getNextToken(const LocatedString *str, int *index);
	bool isSplitChar(wchar_t c) const;

};


#endif
