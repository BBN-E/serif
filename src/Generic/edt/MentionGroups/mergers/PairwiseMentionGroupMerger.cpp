// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"

bool PairwiseMentionGroupMerger::shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	// iterate over all pairs of mentions drawn from g1 and g2.
	for (MentionGroup::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1) {
		for (MentionGroup::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2) {
			if (shouldMerge(*it1, *it2, cache))
				return true;
		}
	}
	return false;
}
