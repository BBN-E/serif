// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_POINTER_MERGER_H
#define MENTION_POINTER_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/** This pairwise merger will merge a mention m1 with a mention m2 if m1 has
  * a feature (with the given name) containing a pointer to m2.
  */
class MentionPointerMerger : public PairwiseMentionGroupMerger {
public:
	MentionPointerMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints);
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
 private:
	bool shouldMergeOneDirection(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
	Symbol _extractorName;
	Symbol _featureName;
};

#endif
