// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_IDFTOKENS_H
#define ar_IDFTOKENS_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Token.h"
#include "Generic/common/limits.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"

class IdFTokens {

private:
	int _tokMap[MAX_SENTENCE_TOKENS*2];
	LexicalToken* _idfTokens[MAX_SENTENCE_TOKENS*2];
	int _numTokens;
	//for substrings
	wchar_t _clitic[2];	
	wchar_t _subword[76];
	bool isIdFClitic(wchar_t c);
public:
	IdFTokens();
	~IdFTokens();
	int getNumTokens();
	const LexicalToken* getToken(int tokNum);
	int getSerifTokenNum(int tokNum);
	void tokenize(const TokenSequence* serifTokens);
	void resetTokens();
};
#endif
