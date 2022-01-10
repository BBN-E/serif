// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_RELATION_TRAINER_H
#define MAXENT_RELATION_TRAINER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/discTagger/DTFeature.h"

class DTFeatureTypeSet;
class DTTagSet;
class DTRelationSet;
class RelationObservation;
class MaxEntModel;
class StateLoader;
class MorphologicalAnalyzer;

class Parse;
class MentionSet;
class PropositionSet;
class DTRelSentenceInfo;
class TrainingLoader;
class HYInstanceSet;


class SentenceTheory;

class MaxEntRelationTrainer {
public:
	MaxEntRelationTrainer();
	~MaxEntRelationTrainer();

	void train();
	void devTest();

private:
	TrainingLoader *_trainingLoader;
	int loadTrainingData();
	SentenceTheory* loadNextTrainingSentence();
	void unloadSentence(DTRelSentenceInfo* sentenceInfo);
	bool _do_not_cache_sentences;

	enum { TRAIN, DEVTEST, ADD_FEATURES};
	void walkThroughSentence(int index, int mode);
	void walkThroughHighYieldAnnotation(int mode);
	void writeWeights();
	void dumpTrainingParameters(UTF8OutputStream &out);

	// Note: filter mode is not yet added to the High Yield training
	std::string _filter_model_file;
	bool _filter_mode;
	Symbol _filterTag;
	Symbol getModelTagSymbol(const Symbol symbol);

	UTF8OutputStream _devTestStream;
	int _correct;
	int _missed;
	int _spurious;
	int _wrong_type;

	int _num_sentences;
	int _beam_width;
	
	DTRelSentenceInfo **_sentenceInfo;

	int _pruning;
	int _percent_held_out;
	int _mode;
	int _max_iterations;
	double _variance;
	double _likelihood_delta;
	int _stop_check_freq;
	std::string _train_vector_file;
	std::string _test_vector_file;
	
	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;

	MaxEntModel *_decoder;
	DTFeature::FeatureWeightMap *_weights;

	StateLoader *_stateLoader;
	RelationObservation *_observation;
	MorphologicalAnalyzer *_morphAnalysis;

	// for training with high yield annotation
	bool _use_high_yield_annotation;
	HYInstanceSet *_highYieldPool;
	HYInstanceSet *_highYieldAnnotation;

	bool _set_secondary_parse;
};

#endif
