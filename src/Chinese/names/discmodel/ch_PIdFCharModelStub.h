// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PIDF_CHAR_MODEL_STUB_H
#define CH_PIDF_CHAR_MODEL_STUB_H

#include "Chinese/names/discmodel/ch_PIdFCharSentence.h"

class DTFeatureTypeSet;
class DTTagSet;
class DocTheory;
class TokenObservation;
class IdFWordFeatures;
class PDecoder;
class NameSpan;

class PIdFCharModel {
private:
	DTTagSet *_tagSet;
public:

	PIdFCharModel();
	PIdFCharModel(char* tag_set_file, char* features_file, 
				  char* model_file, bool learn_transitions = false);

	~PIdFCharModel();

	DTTagSet *getTagSet() { return _tagSet; }

	void decode(PIdFCharSentence &sentence);
	void decode(PIdFCharSentence &sentence, double &margin);

	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	void resetForNewDocument(DocTheory *docTheory = 0) {};

	void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word) {};

	NameTheory *makeNameTheory(PIdFCharSentence &sentence);
};





#endif
