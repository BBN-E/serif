// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PERSON_NAME_MERGER_H
#define EN_PERSON_NAME_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"
#include "Generic/common/AttributeValuePair.h"

#include <vector>

/**
  *  Merges PER mentions when the full string of one mention matches the first or last 
  *  name of the other - e.g. "Smith" and "John Smith".
  */
class EnglishPersonNameMerger : public PairwiseMentionGroupMerger {
public:
	EnglishPersonNameMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 matches the first/last name of m2, or vice versa */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
	
	/** Returns true if at least one pair of features from featureSet1 and featureSet2 matches. */
	bool findValueMatch(const Mention *m1, const Mention *m2, 
	                    std::vector<AttributeValuePair_ptr> featureSet1, 
	                    std::vector<AttributeValuePair_ptr> featureSet2) const;
};

#endif
