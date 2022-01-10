// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef COPULA_MERGER_H
#define COPULA_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges pairs of mentions connected by a copula.
  */
class CopulaMerger : public PairwiseMentionGroupMerger {
public:
	CopulaMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 and m2 are connected by a copula proposition. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
