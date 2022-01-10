// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_MODEL_H
#define P_IDF_MODEL_H

#include "common/limits.h"
#include "common/Symbol.h"
#include "names/discmodel/PIdFSentence.h"

class DTFeatureTypeSet;
class DTTagSet;
class DTObservation;
class TokenObservation;
class PDecoder;
class DocTheory;
class NameTheory;
class IdFWordFeatures;
class NgramScoreTable;

class PIdFModel {
private:
	DTTagSet *_tagSet;
public:
	enum output_mode_e {TACITURN_OUTPUT, VERBOSE_OUTPUT};
	enum model_mode_e {TRAIN, TRAIN_AND_DECODE, DECODE, SIM_AL, UNSUP, UNSUP_CHILD};

	PIdFModel(model_mode_e mode);
	PIdFModel(model_mode_e mode, char* tag_set_file, 
		char* features_file, char* model_file, char* vocab_file,
		bool learn_transitions = false);
	PIdFModel(model_mode_e mode, char* tag_set_file, 
		char* features_file, char* model_file, char* vocab_file, 
		char* lc_model_file, char* lc_vocab_file,
		bool learn_transitions = false);

	~PIdFModel();

	void setWordFeaturesMode(int mode) {}

	DTTagSet *getTagSet() { return _tagSet; }

	void addTrainingSentencesFromTrainingFile(const char *file = 0) {};
	void addActiveLearningSentencesFromTrainingFile(const char *file = 0) {};
	
	void train() {};
	
	void doActiveLearning() {};
	void seedALTraining() {};
	void doUnsupervisedTrainAndSelect() {};

	 void decode() {};
	 void decode(UTF8InputStream &in, UTF8OutputStream &out) {};
	 void decode(PIdFSentence &sentence);
	 void decode(PIdFSentence &sentence, double& margin);
	
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	void resetForNewDocument(DocTheory *docTheory = 0) {};

	
	void writeModel(char* str) {};
	void freeWeights() {};
	void finalizeWeights() {};

	static void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word, 
		//bool word_is_in_vocab,
		int wordCount,
		bool lowercased_word_is_in_vocab,
		bool use_lowercase_clusters,
		NgramScoreTable* bigram_counts) {};

	NameTheory *makeNameTheory(PIdFSentence &sentence);

	void useLowercaseDecoder() {};
	void useDefaultDecoder() {};

};





#endif
