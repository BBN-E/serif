// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PAIRWISE_MENTION_GROUP_CONSTRAINT_H
#define PAIRWISE_MENTION_GROUP_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

class Mention;

/**
  *  Base class for constraints that prevent the merging of two MentionGroups 
  *  on the basis of evidence about a pair of mentions, one from each MentionGroup.
  */
class PairwiseMentionGroupConstraint : public MentionGroupConstraint {
protected:
	/** Returns true if any pair of mentions from g1 and g2 violate this constraint. */
	bool violatesMergeConstraint(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;

	/** Returns true if mentions m1 and m2 violate this constraint. */
	virtual bool violatesMergeConstraint(const Mention* m1, const Mention* m2, LinkInfoCache& cache) const = 0;
};

#endif

