// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_MORPH_DECODER_H
#define XX_MORPH_DECODER_H

class GenericMorphDecoder : public MorphDecoder {
private:
	friend class GenericMorphDecoderFactory;

public:

	~GenericMorphDecoder() {};

	virtual void putTokenSequenceInAtomizedSentence(const LocatedString& sentenceString, TokenSequence* ts) {};
	virtual void putTokenSequenceInAtomizedSentence(const LocatedString& sentenceString, TokenSequence *ts, 
		CharOffset *constraints, int n_constraints) {};

private:
	GenericMorphDecoder() {};

};

class GenericMorphDecoderFactory: public MorphDecoder::Factory {
	virtual MorphDecoder *build() { return _new GenericMorphDecoder(); }
};



#endif
