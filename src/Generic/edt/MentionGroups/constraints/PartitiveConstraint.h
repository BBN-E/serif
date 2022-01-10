// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARTITIVE_CONSTRAINT_H
#define PARTITIVE_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"

/**
  *  Prevents partitive mentions from being merged into a larger MentionGroup.
  */
class PartitiveConstraint : public PairwiseMentionGroupConstraint {
public:
	PartitiveConstraint() {}
protected:
	/** Returns true if either m1 or m2 is a partitive mention */
	bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
