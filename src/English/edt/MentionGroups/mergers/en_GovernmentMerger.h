// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_GOVERNMENT_MERGER_H
#define EN_GOVERNMENT_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions that share a government proposition as identified by EnglishPreLinker::preLinkGovernment()
  *  e.g. "the government of France", "the Iraqi regime" 
  */
class EnglishGovernmentMerger : public PairwiseMentionGroupMerger {
public:
	EnglishGovernmentMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 refers to the government of m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
