// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MORPH_SELECTOR_H
#define MORPH_SELECTOR_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Offset.h"

#include <wchar.h>

class LocatedString;
class TokenSequence;
class Token;
class MorphDecoder;

class MorphSelector {
public:

	MorphSelector();
	virtual ~MorphSelector();

	void resetForNewSentence(){};//you could set the dictionaries here

	// overwritting the default selectTokenization
	// the default functions only work for Arabic and Korean
	// in xx_morphSelction.h these functions do nothing
	virtual int selectTokenization(const LocatedString *sentenceString, TokenSequence *origTS);
	virtual int selectTokenization(const LocatedString *sentenceString, TokenSequence *origTS,
		                           CharOffset *constraints, int n_constraints);
	
protected:
	void updateTokenSequence(TokenSequence* origTS, int* map, OffsetGroup* start, OffsetGroup* end, 
		Symbol* words, int nwords);

	MorphDecoder* morphDecoder;

	Symbol words[MAX_SENTENCE_TOKENS];
	int map[MAX_SENTENCE_TOKENS];
	OffsetGroup startOffsets[MAX_SENTENCE_TOKENS];
	OffsetGroup endOffsets[MAX_SENTENCE_TOKENS];
	Token *newTokens[MAX_SENTENCE_TOKENS];
	
};

#endif
