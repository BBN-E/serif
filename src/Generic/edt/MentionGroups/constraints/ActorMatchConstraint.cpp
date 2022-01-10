// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/ActorMatchConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"

#include <algorithm>

bool ActorMatchConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	if (!mg1.getEntityType().matchesGPE() || !mg2.getEntityType().matchesGPE())
		return false;

	// Get all confident actor matches for mg1
	std::vector<AttributeValuePair_ptr> mg1Matches = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-actor-match"), Symbol(L"confident-actor-match"));
	std::set<int> mg1MatchSet = MentionGroupUtils::getIntValueSet(mg1Matches);
	if (!mg1MatchSet.empty()) {
		std::vector<AttributeValuePair_ptr> mg2Matches = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-actor-match"), Symbol(L"confident-actor-match"));
		std::set<int> mg2MatchSet = MentionGroupUtils::getIntValueSet(mg2Matches);
		if (!mg2MatchSet.empty()) {
			std::set<int> diffSet;
			std::set_difference(mg1MatchSet.begin(), mg1MatchSet.end(), mg2MatchSet.begin(), mg2MatchSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
						SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by GPE actor match constraint";
				return true;
			}
		}
	}
	return false;
}
