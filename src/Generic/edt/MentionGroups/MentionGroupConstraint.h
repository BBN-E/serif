// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_GROUP_CONSTRAINT_H
#define MENTION_GROUP_CONSTRAINT_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

class MentionGroup;
class LinkInfoCache;

/** 
  *  Base class for constraints on MentionGroup merge operations. 
  */
class MentionGroupConstraint : boost::noncopyable {
public:
	typedef enum { DEFAULT, HIGH_PRECISION } Mode;
	
	virtual ~MentionGroupConstraint() {}

	/** Return true if this constraint would be violated by the merge of mg1 and mg2. */
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const = 0;

protected:
	MentionGroupConstraint() {}
};

typedef boost::shared_ptr<MentionGroupConstraint> MentionGroupConstraint_ptr;

#endif


