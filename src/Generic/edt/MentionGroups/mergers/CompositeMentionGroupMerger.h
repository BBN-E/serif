// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMPOSITE_MENTION_GROUP_MERGER_H
#define COMPOSITE_MENTION_GROUP_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"

#include <vector>

class Symbol;

/**
  *  Represents an ordered collection of MentionGroupMergers.
  */
class CompositeMentionGroupMerger : public MentionGroupMerger {
public:
	CompositeMentionGroupMerger() : MentionGroupMerger(Symbol(L"AnonymousComposite")) {}
	CompositeMentionGroupMerger(Symbol name) : MentionGroupMerger(name) {}
	virtual ~CompositeMentionGroupMerger() {};

	/** Calls merge method for each child merger in turn. */
	void merge(MentionGroupList& groups, LinkInfoCache& cache);

	/** Adds a new child merger to the end of its list. */
	void add(MentionGroupMerger_ptr child) { _children.push_back(child); }

private:
	std::vector<MentionGroupMerger_ptr> _children;

	/** no-op */
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const { return false; }
};


#endif
