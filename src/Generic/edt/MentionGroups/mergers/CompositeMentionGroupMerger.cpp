// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"

#include <boost/foreach.hpp>

void CompositeMentionGroupMerger::merge(MentionGroupList& groups, LinkInfoCache& cache) {
	BOOST_FOREACH(MentionGroupMerger_ptr child, _children) {
		child->merge(groups, cache);
	}
}
