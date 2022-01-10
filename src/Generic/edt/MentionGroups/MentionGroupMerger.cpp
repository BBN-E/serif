// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"

#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"
#include "Generic/common/SessionLogger.h"

bool MentionGroupMerger::isLegalMerge(const MentionGroup& g1, const MentionGroup& g2) {
	if (_max_sentence_distance < 0)
		return true;

	int s1 = g1.getFirstSentenceNumber();
	int s2 = g2.getFirstSentenceNumber();
	int e1 = g1.getLastSentenceNumber();
	int e2 = g2.getLastSentenceNumber();

	if (e1 + _max_sentence_distance < s2)
		return false;

	if (e2 + _max_sentence_distance < s1)
		return false;

	return true;
}

void MentionGroupMerger::merge(MentionGroupList& groups, LinkInfoCache& cache) {
	for (MentionGroupList::iterator it1 = groups.begin(); it1 != groups.end(); ++it1) {
		MentionGroupList::iterator it2 = it1;
		for (++it2; it2 != groups.end(); ) {
			if (isLegalMerge(**it1, **it2) && shouldMerge(**it1, **it2, cache)) {
				if (_constraints.get() != 0 && !_constraints->violatesMergeConstraint(**it1, **it2, cache)) {
					SessionLogger::dbg("MentionGroupMerger") << "Merge identified.";
					(**it1).merge(**it2, this);
					it2 = groups.erase(it2);
				} else {
					++it2;
				}
			} else {
				++it2;
			}
		}
	}
}
