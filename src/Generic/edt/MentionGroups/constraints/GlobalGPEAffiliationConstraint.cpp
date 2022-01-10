// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/GlobalGPEAffiliationConstraint.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include <algorithm>
#include <boost/foreach.hpp>

bool GlobalGPEAffiliationConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	if (!mg1.getEntityType().matchesPER() || !mg2.getEntityType().matchesPER())
		return false;

	// Get all GPE modifiers for mg1, names only
	std::vector<AttributeValuePair_ptr> mg1Mods = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-gpe"), Symbol(L"modifier"));
	std::set<Symbol> mg1ModSet = MentionGroupUtils::getSymbolValueSet(mg1Mods);
	if (!mg1ModSet.empty()) {
		std::vector<AttributeValuePair_ptr> mg2Affs = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-gpe"), Symbol(L"affiliation"));
		std::set<Symbol> mg2AffSet = MentionGroupUtils::getSymbolValueSet(mg2Affs); 
		if (!mg2AffSet.empty()) {
			std::set<Symbol> diffSet;
			std::set_difference(mg1ModSet.begin(), mg1ModSet.end(), mg2AffSet.begin(), mg2AffSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
						SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by global GPE affiliation constraint";
				return true;
			}
		}
	}

	// Get all GPE modifiers for mg2, names only
	std::vector<AttributeValuePair_ptr> mg2Mods = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-gpe"), Symbol(L"modifier"));
	std::set<Symbol> mg2ModSet = MentionGroupUtils::getSymbolValueSet(mg2Mods);
	if (!mg2ModSet.empty()) {
		std::vector<AttributeValuePair_ptr> mg1Affs = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-gpe"), Symbol(L"affiliation"));
		std::set<Symbol> mg1AffSet = MentionGroupUtils::getSymbolValueSet(mg1Affs); 
		if (!mg1AffSet.empty()) {
			std::set<Symbol> diffSet;
			std::set_difference(mg2ModSet.begin(), mg2ModSet.end(), mg1AffSet.begin(), mg1AffSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
						SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by global GPE affiliation constraint";
				return true;
			}
		}
	}

	// Get all GPE modifiers for mg1, country names only, compare to actor-affiliations (which are also country names)
	mg1Mods = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-gpe"), Symbol(L"country-actor-modifier"));
	mg1ModSet = MentionGroupUtils::getSymbolValueSet(mg1Mods);
	if (!mg1ModSet.empty()) {
		std::vector<AttributeValuePair_ptr> mg2Affs = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-gpe"), Symbol(L"country-actor-affiliation"));
		std::set<Symbol> mg2AffSet = MentionGroupUtils::getSymbolValueSet(mg2Affs); 
		if (!mg2AffSet.empty()) {
			std::set<Symbol> diffSet;
			std::set_difference(mg1ModSet.begin(), mg1ModSet.end(), mg2AffSet.begin(), mg2AffSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
					SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by global GPE affiliation constraint (country-actor-affiliation)";
				return true;
			}
		}
	}

	// Get all GPE modifiers for mg2, country names only, compare to actor-affiliations (which are also country names)
	mg2Mods = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-gpe"), Symbol(L"country-actor-modifier"));
	mg2ModSet = MentionGroupUtils::getSymbolValueSet(mg2Mods);
	if (!mg2ModSet.empty()) {
		std::vector<AttributeValuePair_ptr> mg1Affs = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-gpe"), Symbol(L"country-actor-affiliation"));
		std::set<Symbol> mg1AffSet = MentionGroupUtils::getSymbolValueSet(mg1Affs); 
		if (!mg1AffSet.empty()) {
			std::set<Symbol> diffSet;
			std::set_difference(mg2ModSet.begin(), mg2ModSet.end(), mg1AffSet.begin(), mg1AffSet.end(),
				std::inserter(diffSet, diffSet.end()));
			if (!diffSet.empty()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
					SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by global GPE affiliation constraint (country-actor-affiliation)";
				return true;
			}
		}
	}

	return false;
}
