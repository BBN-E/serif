// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/LocalGPEAffiliationConstraint.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/Assert.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"

#include <boost/foreach.hpp>

bool LocalGPEAffiliationConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	// Don't want to apply this to GPEs
	if (mg1.getEntityType() == EntityType::getGPEType() || mg2.getEntityType() == EntityType::getGPEType())
		return false;

	// Get all GPE modifiers for our MentionGroups, names only
	std::vector<AttributeValuePair_ptr> mg1Mods = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-gpe"), Symbol(L"modifier"));
	std::vector<AttributeValuePair_ptr> mg2Mods = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-gpe"), Symbol(L"modifier"));
	std::set<Symbol> mg1ModSet = MentionGroupUtils::getSymbolValueSet(mg1Mods);
	std::set<Symbol> mg2ModSet = MentionGroupUtils::getSymbolValueSet(mg2Mods);

	if (!mg1ModSet.empty()) {
		if (!mg2ModSet.empty()) {
			std::set<Symbol> diffSet;
			// mg1ModSet - mg2ModSet must be empty
			std::set_difference(mg1ModSet.begin(), mg1ModSet.end(), mg2ModSet.begin(), mg2ModSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
						SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by local GPE affiliation constraint";
				return true;
			}
			// mg2ModSet - mg1ModSet must be empty
			std::set_difference(mg2ModSet.begin(), mg2ModSet.end(), mg1ModSet.begin(), mg1ModSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
						SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by local GPE affiliation constraint";
				return true;
			}
		}
	}
	
	return false;
}
