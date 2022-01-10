// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_TYPE_CONSTRAINT_H
#define ENTITY_TYPE_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"

/**
  *  Prevents merges between MentionGroups that don't share the same recognized EntityType.
  */
class EntityTypeConstraint : public PairwiseMentionGroupConstraint {
public:
	EntityTypeConstraint() {}
protected:
	/** Returns true if m1 and m2 have conflicting recognized entity types. */
	bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
