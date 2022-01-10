// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef DUMMY_MENTION_GROUP_MERGER_H
#define DUMMY_MENTION_GROUP_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"

/**
  *  No-op merger - for testing purposes. 
  */
class DummyMentionGroupMerger : public MentionGroupMerger {
public:
	DummyMentionGroupMerger(MentionGroupConstraint_ptr constraints) : MentionGroupMerger(Symbol(L"Dummy"), constraints) {}
protected:
	/** Returns false */
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const { return false; }
};

#endif
