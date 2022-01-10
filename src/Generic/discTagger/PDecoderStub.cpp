// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "discTagger/DTTagSet.h"
#include "discTagger/DTObservation.h"
#include "discTagger/DTFeatureType.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "discTagger/PDecoder.h"


PDecoder::PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   DTFeature::FeatureWeightMap *weights, bool add_hyp_features,
				   bool compute_lazy_sum)
	: _tagSet(tagSet)
{}

void PDecoder::decode(int n_obs, DTObservation **observations, Symbol *tags) {
	for (int i = 0; i < n_obs; i++) {
		tags[i] = _tagSet->getNoneSTTag();
	}
}

double PDecoder::decode(int n_obs, DTObservation **observations,
					  int *tags)
{
	for (int i = 0; i < n_obs; i++) {
		tags[i] = _tagSet->getNoneTagIndex();
	}
	return 0.0;
}
