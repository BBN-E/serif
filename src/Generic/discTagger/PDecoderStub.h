// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_DECODER_H
#define P_DECODER_H

#include "common/Symbol.h"
#include "discTagger/DTFeature.h"

class DTTagSet;
class DTObservation;
class DTFeatureTypeSet;

class PDecoder {
private:
	DTTagSet *_tagSet;

public:
	PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
			 DTFeature::FeatureWeightMap *weights,
			 bool add_hyp_features = true,
			 bool compute_lazy_sum = false);
    
	void decode(int n_observations, DTObservation **observations,
				Symbol *tags);
	double decode(int n_observations, DTObservation **observations,
				int *tags);

	bool train(int n_obs, DTObservation **observations, 
		int *answer, int n_obs_to_ignore) { return true; };
	
	long getNHypotheses() const { return 0; }
};

#endif

