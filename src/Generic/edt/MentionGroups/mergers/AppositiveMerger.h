// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef APPOSITIVE_MERGER_H
#define APPOSITIVE_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/** 
  *  Merges pairs of mentions in appositive relationships. 
  */
class AppositiveMerger : public PairwiseMentionGroupMerger {
public:
	AppositiveMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 and m2 are connected by an appositive relationship. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
