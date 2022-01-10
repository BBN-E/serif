// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef FEATURE_UNIQUE_MATCH_MERGER_H
#define FEATURE_UNIQUE_MATCH_MERGER_H

#include "Generic/common/Symbol.h"
#include "Generic/edt/MentionGroups/MentionGroupMerger.h"

class FeatureExactMatchMerger;
class Mention;

/**
  *  Merges two MentionGroups when mentions from each group have matching
  *  feature values for a specified feature type AND that feature value
  *  does not appear in any other MentionGroup.
  */
class FeatureUniqueMatchMerger : public MentionGroupMerger {
public:
	/** Construct a merger for the feature specified by the given extractor and feature names (e.g. "Mention-person-name-variations" and "last") */
	FeatureUniqueMatchMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints);

protected:
	Symbol _extractorName;
	Symbol _featureName;

	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;
	virtual bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache, Symbol& value) const; 
	
	/** Returns true if value is the key value for mentions in g1 and g2 and no other mentions in the document. */
	bool checkForUniqueness(Symbol value, const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;
};

#endif
