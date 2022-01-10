// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1_RELATION_TRAINER_H
#define P1_RELATION_TRAINER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/discTagger/DTFeature.h"

#include "Generic/common/NgramScoreTable.h"

class DTFeatureTypeSet;
class DTTagSet;
class DTRelationSet;
class RelationObservation;
class P1Decoder;
class StateLoader;
class MorphologicalAnalyzer;
class TrainingLoader;

class Parse;
class MentionSet;
class PropositionSet;
class DTRelSentenceInfo;
class HYInstanceSet;




class P1RelationTrainer {
public:
	P1RelationTrainer();
	~P1RelationTrainer();

	void train();
	void devTest();
	void getSyntacticFeatures();

private:
	TrainingLoader *_trainingLoader;
	int loadTrainingData();

	bool _seed_features;
	bool _add_hyp_features;
	int _weightsum_granularity;
	double _overgen_percentage;
	// Take the actual average of the weights instead of adding once every period set by _weightsum_granularity, and _hy...
	bool _real_averaged_mode;

	enum { TRAIN, DEVTEST, ADD_FEATURES, GET_SYNTAX};
	void trainEpoch();
	void walkThroughSentence(int index, int mode, int& num_candidates, int& num_correct);
	void walkThroughSentence(int index, int mode);
	void walkThroughHighYieldAnnotation(int mode, int& num_candidates, int& num_correct);
	void walkThroughHighYieldAnnotation(int mode);
	void writeWeights(int epoch = -1);
	void dumpTrainingParameters(UTF8OutputStream &out);

	void writeHtmlMentions(RelationObservation* observation, UTF8OutputStream &debugStream);

	UTF8OutputStream _devTestStream;
	int _correct;
	int _missed;
	int _spurious;
	int _wrong_type;

	int _num_sentences;
	int _beam_width;

	bool _include_ident_relations;
	
	DTRelSentenceInfo **_sentenceInfo;

	int _epochs;
	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;

	P1Decoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;
	DTFeature::FeatureWeightMap *_weightSums;

	
	StateLoader *_stateLoader;
	RelationObservation *_observation;
	MorphologicalAnalyzer *_morphAnalysis;

	//for storing syntactic features
	NgramScoreTable* _distBtwnMent;
	NgramScoreTable* _posBtwnMent;
	NgramScoreTable* _parsePathBtwnMent;
	NgramScoreTable* _mentBtwnRelMent;

	int _pair_count;
	int _non_none_count;
	

	// for training with high yield annotation
	bool _use_high_yield_annotation;
	HYInstanceSet *_highYieldPool;
	HYInstanceSet *_highYieldAnnotation;
	int _hy_weightsum_granularity;

	bool _set_secondary_parse;

	bool _debug_devtest_mistakes;
};

#endif
