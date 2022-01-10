// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOCAL_GPE_AFFILIATION_CONSTRAINT_H
#define LOCAL_GPE_AFFILIATION_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

/**
  *  Prevents merges between pairs of MentionGroups when there are GPEs modifiers
  *  of one group that don't occur anywhere in the GPE modifiers of the second
  *  group.
  */
class LocalGPEAffiliationConstraint : public MentionGroupConstraint {
public:
	LocalGPEAffiliationConstraint() {}

	/** Returns true if mg1 contains GPE modifiers that are not present in mg2, or vice versa. */
	bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
};

#endif
