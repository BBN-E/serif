// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMPOSITE_MENTION_GROUP_CONSTRAINT_H
#define COMPOSITE_MENTION_GROUP_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

#include <vector>

/**
  *  Represents an ordered collection of MentionGroupConstraints.
  */
class CompositeMentionGroupConstraint : public MentionGroupConstraint {
public:
	virtual ~CompositeMentionGroupConstraint() {};

	/** Returns true if any of its child constraint are violated. */
	bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;

	/** Adds a new child constraint to the end of its list. */
	void add(MentionGroupConstraint_ptr child) { _children.push_back(child); }
private:
	std::vector<MentionGroupConstraint_ptr> _children;
};


#endif
