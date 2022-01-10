// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SAME_DEFINITE_DESCRIPTION_MERGER_H
#define SAME_DEFINITE_DESCRIPTION_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges two MentionGroups when mentions from each group both start
  *  with a definite article ("the") and have identical text.
  */
class SameDefiniteDescriptionMerger : public PairwiseMentionGroupMerger {
public:
	SameDefiniteDescriptionMerger(MentionGroupConstraint_ptr constraints);
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
