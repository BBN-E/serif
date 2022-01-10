// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_MORPH_DECODER_H
#define KR_MORPH_DECODER_H

#include "Generic/morphSelection/MorphDecoder.h"

class TokenSequence;
class KoreanMorphologicalAnalyzer;

class KoreanMorphDecoder: public MorphDecoder {
public:

	KoreanMorphDecoder() {}
	~KoreanMorphDecoder() {}

	void putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence* ts);
	void putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence *ts, 
		CharOffset *constraints, int n_constraints);
};


#endif
