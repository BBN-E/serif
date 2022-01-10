// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PNPCHUNK_DECODER_H
#define PNPCHUNK_DECODER_H

#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/theories/NPChunkTheory.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTFeatureTypeSet;
class DTTagSet;
class TokenPOSObservation;
class IdFWordFeatures;
class PDecoder;
class TokenSequence;
class PNPChunkSentence;
class DTObservation;


class PNPChunkDecoder {
public:
	PNPChunkDecoder();
	~PNPChunkDecoder();

	void decode();
	void decode(UTF8InputStream &in, UTF8OutputStream &out);
	void decode(const TokenSequence* tokens, const Symbol* pos, PNPChunkSentence*& sentence);

	void devTest();
	void devTest(UTF8InputStream &in, UTF8OutputStream &out);

	static void populateObservation(TokenPOSObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, Symbol pos, bool first_word);
	static void populateObservation(TokenPOSObservation *observation,
		IdFWordFeatures *wordFeatures, const Token* wordtoken, Symbol pos, bool first_word);
	DTTagSet* getTagSet(){return _tagSet;};


private:

	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;
	
	PDecoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;

	std::vector<DTObservation*> _observations;
	/*
	DTObservation *_observations[MAX_SENTENCE_TOKENS+4];
	TokenPOSObservation *_obsArray;
	*/
};

#endif

