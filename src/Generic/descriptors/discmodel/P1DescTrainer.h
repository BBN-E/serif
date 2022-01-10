// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1_DESC_TRAINER_H
#define P1_DESC_TRAINER_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/theories/PropositionSet.h"

class DTFeatureTypeSet;
class DTTagSet;
class DTRelationSet;
class DescriptorObservation;
class P1Decoder;
class MaxEntModel;

class Parse;
class Mention;
class MentionSet;
class NPChunkTheory;
class PropostionSet;
class MorphologicalAnalyzer;

#define MAX_BATCHES 10

class P1DescTrainer {
public:
	P1DescTrainer();
	~P1DescTrainer();
	

	void train();
	void devTest();

private:
	typedef enum { TRAIN, DEVTEST, ADD_FEATURES} Mode;
	MorphologicalAnalyzer* _morphAnalysis;

	int loadTrainingData(const char* file_list, int n_sentences = 0);
	int loadBatchTrainingData(const char* file_list, int batch_num);

	void trainEpoch();
	void checkHWConsistency();

	int _n_training_batches;
	int _n_sent_per_batch[MAX_BATCHES];
	std::string _training_file_basename;
	void makeTrainingSubfiles(const char* training_list, const char* sublist_prefix);

	double walkThroughSentence(int index, Mode mode);
	void writeWeights(int epoch = -1);
	void dumpTrainingParameters(UTF8OutputStream &out);

	bool isInTrainingSet(const Mention *mention);
	int getEntType(const Mention *mention, const MentionSet* mentSet);


	UTF8OutputStream _devTestStream;
	int _correct;
	int _missed;
	int _spurious;
	int _wrong_type;

	enum { DESC_CLASSIFY, PREMOD_CLASSIFY, DESC_PREMOD_CLASSIFY, PRONOUN_CLASSIFY } _task;
	enum { P1, MAXENT } _model_type;
	bool _seed_features;
	bool _add_hyp_features;
	bool _use_wordnet;
	int _weightsum_granularity;
	int _epochs;

	int _max_iterations;
	int _stop_check_freq;
	int _percent_held_out;
	double _likelihood_delta;
	double _variance;

	int _num_sentences;
	int _beam_width;
	Parse** _parses;
	NPChunkTheory** _npChunks;
	MentionSet** _mentionSets;
	PropositionSet** _propSets;

	std::string _training_file;
	std::string _subtraining_file_prefix;
	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;

	P1Decoder *_p1Decoder;
	MaxEntModel *_maxentDecoder;
	DTFeature::FeatureWeightMap *_weights;

	DescriptorObservation *_observation;
	TrainingLoader *_trainingLoader;

	std::string _training_vectors_file;
	std::string _heldout_vectors_file;

};

#endif
