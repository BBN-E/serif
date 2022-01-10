// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MORPH_DECODER_H
#define AR_MORPH_DECODER_H

#include "Generic/morphSelection/MorphDecoder.h"

class TokenSequence;

class ArabicMorphDecoder : public MorphDecoder {
private:
	friend class ArabicMorphDecoderFactory;

public:

	void putTokenSequenceInAtomizedSentence(const LocatedString& sentenceString, TokenSequence* ts);
	void putTokenSequenceInAtomizedSentence(const LocatedString& sentenceString, TokenSequence *ts, 
		CharOffset *constraints, int n_constraints);

private:
	ArabicMorphDecoder() {};

};

class ArabicMorphDecoderFactory: public MorphDecoder::Factory {
	virtual MorphDecoder *build() { return _new ArabicMorphDecoder(); }
};

#endif
