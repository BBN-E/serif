// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"

#include <boost/foreach.hpp>

bool CompositeMentionGroupConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	BOOST_FOREACH(MentionGroupConstraint_ptr child, _children) {
		if (child->violatesMergeConstraint(mg1, mg2, cache)) 
			return true;
	}
	return false;
}

