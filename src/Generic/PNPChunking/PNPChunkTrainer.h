// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_NPChunk_TRAINER_H
#define P_NPChunk_TRAINER_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"

class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class NPChunkWordFeatures;
class PDecoder;
class IdFWordFeatures;


class PNPChunkTrainer {
public:
	PNPChunkTrainer();
	~PNPChunkTrainer();

	void train();

private:
	void trainEpoch();
	void writeWeights(int epoch = -1);
	void writeWeights(const char* str);
	void writeLazySumWeights(long n_hypotheses, int epoch = -1);
	void writeLazySumWeights(const char* str, long n_hypotheses);
	void readTrainingFileList(const char *training_list_file);

	int _epochs;
	std::vector<std::string> _training_files;
	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;

	PDecoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;
	//DTFeature::FeatureWeightMap *_weightSums;

	bool _print_last_weights_after_every_epoch;
	bool _print_sum_weights_after_every_epoch;
};

#endif
