// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_GROUP_MERGER_H
#define MENTION_GROUP_MERGER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <set>
#include <vector>
#include <Generic/edt/MentionGroups/MentionGroup.h>

class LinkInfoCache;
class MentionGroupConstraint;
typedef boost::shared_ptr<MentionGroupConstraint> MentionGroupConstraint_ptr;

/** 
  *  Abstract base class for merge operations over MentionGroups. 
  *
  *  Most derived classes will only need to implement the shouldMerge method, since
  *  a base class implementation of merge() is provided.
  */
class MentionGroupMerger : boost::noncopyable {
public:
	MentionGroupMerger(Symbol name, MentionGroupConstraint_ptr constraints=MentionGroupConstraint_ptr()) : _name(name), _constraints(constraints) {
		_max_sentence_distance = ParamReader::getOptionalIntParamWithDefaultValue("coref_max_sentence_distance", -1);
	}
	virtual ~MentionGroupMerger() {}

	/** Test and apply all valid merges to the set of groups, subject to constraints. 
		If you override this function you MUST include a check to isLegalMerge() in your
		re-implementation. */
	virtual void merge(MentionGroupList& groups, LinkInfoCache& cache);

	/** Should these two mention groups be even considered for a merge */
	bool isLegalMerge(const MentionGroup& g1, const MentionGroup& g2);

	/** Return the name of this merger. */
	Symbol getName() const { return _name; }

protected:
	/** Set of constraints to be checked for each potential merge. */
	MentionGroupConstraint_ptr _constraints;

	/** The name of this merger */
	Symbol _name;

	/** Return true if g1 and g2 should be merged. */
	virtual bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const = 0;

private:
	/** Sentence distance constraint for considering matches */
	int _max_sentence_distance;
};

typedef boost::shared_ptr<MentionGroupMerger> MentionGroupMerger_ptr;

#endif
