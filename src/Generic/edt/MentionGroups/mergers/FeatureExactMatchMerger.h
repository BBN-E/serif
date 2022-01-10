// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef FEATURE_EXACT_MATCH_MERGER_H
#define FEATURE_EXACT_MATCH_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges two MentionGroups when mentions from each group have matching
  *  feature values for a specified feature type.
  */
class FeatureExactMatchMerger : public PairwiseMentionGroupMerger {
public:
	/** Construct a merger for the feature specified by the given extractor and feature names (e.g. "Mention-person-name-variations" and "first-last") */
	FeatureExactMatchMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints);
protected:
	Symbol _extractorName;
	Symbol _featureName;

	/** Returns true if the value of _extractorName:_featureName for m1 and m2 matches. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
