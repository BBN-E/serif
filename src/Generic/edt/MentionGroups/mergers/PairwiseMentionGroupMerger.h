// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PAIRWISE_MENTION_GROUP_MERGER_H
#define PAIRWISE_MENTION_GROUP_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"

class Mention;
class MentionGroup;
class MentionGroupConstraint;
class LinkInfoCache;
class Symbol;

/**
  *  Base class for mergers that merge two MentionGroups on the basis
  *  of evidence about a pair of mentions, one from each MentionGroup.
  */
class PairwiseMentionGroupMerger : public MentionGroupMerger {
protected:
	PairwiseMentionGroupMerger(Symbol name, MentionGroupConstraint_ptr constraints=MentionGroupConstraint_ptr()) : MentionGroupMerger(name, constraints) {}

	/** Returns true if any pair of mentions from g1 and g2 indicate a merge. */
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;

	/** Returns true if mentions m1 and m2 should be merged. */
	virtual bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const = 0; 
};

#endif
