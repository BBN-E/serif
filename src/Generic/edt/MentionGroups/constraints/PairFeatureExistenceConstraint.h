// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PAIR_FEATURE_EXISTENCE_CONSTRAINT_H
#define PAIR_FEATURE_EXISTENCE_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"
#include "Generic/common/Symbol.h"

/**
  *  Prevents merges between two MentionGroups when at least one pair of
  *  mentions across the groups is associated with a specified MentionPair feature.
  */
class PairFeatureExistenceConstraint : public PairwiseMentionGroupConstraint {
public:
	PairFeatureExistenceConstraint(Symbol extractorName, Symbol featureName) : 
	  _extractorName(extractorName), _featureName(featureName) {};

protected:
	Symbol _extractorName;
	Symbol _featureName;

	/** Returns true if the MentionPair(m1, m2) is associated with feature _featureName */
	bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
