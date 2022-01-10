// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PIDF_DECODER_H
#define PIDF_DECODER_H

#include "common/Symbol.h"
#include "discTagger/DTFeature.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFSentence;
class IdFWordFeatures;
class PDecoder;
class NameTheory;
class TokenSequence;
class PIdFSentence;
class DocTheory;


class PIdFDecoder {
public:
	/** This constructor initializes using ParamReader. */
	PIdFDecoder();

	/** This constructor will make a ParamReader-less PIdFDecoder if you
	  * specify all the arguments.
	  *
	  * NOTE: if you specify wordFeatures, that object will be deleted
	  * when the PIdFDecoder instance is destroyed.
	  */
	PIdFDecoder(const char *tag_set_file, const char *features_file,
				const char *model_file, const char *word_clusters_file = 0,
				IdFWordFeatures *wordFeatures = 0);

	~PIdFDecoder();

	DTTagSet *getTagSet() { return _tagSet; }

	/** This method decodes using the input and output files specified
	  * by the parameter file. */
	void decode();

	/** This method decodes using provided streams */
	void decode(UTF8InputStream &in, UTF8OutputStream &out);

	/** This method decodes on a PIdFSentence by storing its answers
	  * in the PIdFSentence instance. This is the decode method to use
	  * when you do your own I/O -- that's you, Marc. */
	void decode(PIdFSentence &sentence);

	// for use as a Serif stage (as opp. to standalone):
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	void resetForNewDocument(DocTheory *docTheory = 0);

	// this is public because it is used by the trainer as well.
	static void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word, 
		bool lowercase = false);

private:
	NameTheory *makeNameTheory(PIdFSentence &sentence);

private:
	static Symbol _NONE_ST;
	static Symbol _NONE_CO;

	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;
	bool _interleave_tags;	//this changes the order tags are processed
	PDecoder *_decoder;
	PDecoder *_defaultDecoder;
	DTFeature::FeatureWeightMap *_defaultWeights;
	PDecoder *_upperCaseDecoder;
	DTFeature::FeatureWeightMap *_ucWeights;
	PDecoder *_lowerCaseDecoder;
	DTFeature::FeatureWeightMap *_lcWeights;
	bool _learn_transitions_from_training;
	void readTransitions(char* transitionfile);
};

#endif

