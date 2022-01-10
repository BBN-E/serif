// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef GLOBAL_GPE_AFFILIATION_CONSTRAINT_H
#define GLOBAL_GPE_AFFILIATION_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

/**
  *  Prevents merges between PER MentionGroups when there are GPEs that are known
  *  to be affiliated with at least one of the names in one group that don't
  *  occur anywhere in the GPE modifiers of the second group.
  */
class GlobalGPEAffiliationConstraint : public MentionGroupConstraint {
public:
	GlobalGPEAffiliationConstraint() {}

	/** 
	  * Returns true if mg1 and mg2 are both PER entities and mg1 contains GPE modifiers that don't
	  * show up in the list of known GPE affiliations of mg2, or vice versa. 
	  */
	bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
};

#endif
