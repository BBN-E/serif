// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef DUMMY_MENTION_GROUP_CONSTRAINT_H
#define DUMMY_MENTION_GROUP_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

/**
  *  No-op constraint - for testing purposes. 
  */
class DummyMentionGroupConstraint : public MentionGroupConstraint {
public:
	/** Returns false */
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const { return false; }
};

#endif
