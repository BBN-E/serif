// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"

bool PairwiseMentionGroupConstraint::violatesMergeConstraint(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	for (MentionGroup::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1) {
		for (MentionGroup::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2) {
			if (violatesMergeConstraint(*it1, *it2, cache))
				return true;
		}
	}
	return false;
}
