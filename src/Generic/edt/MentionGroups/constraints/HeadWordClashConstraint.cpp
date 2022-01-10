// Copyright (c) 2016 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/edt/MentionGroups/constraints/HeadWordClashConstraint.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"


bool HeadWordClashConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> mg1HeadWords = cache.getMentionFeaturesByName(mg1, Symbol(L"Mention-head-word"), Symbol(L"head-word"));
	std::set<Symbol> mg1HeadWordSet = MentionGroupUtils::getSymbolValueSet(mg1HeadWords);
	std::vector<AttributeValuePair_ptr> mg2HeadWords = cache.getMentionFeaturesByName(mg2, Symbol(L"Mention-head-word"), Symbol(L"head-word"));
	std::set<Symbol> mg2HeadWordSet = MentionGroupUtils::getSymbolValueSet(mg2HeadWords);

	if (mg1HeadWordSet.empty() || mg2HeadWordSet.empty())
		return false;

	std::set<Symbol> intersectionSet;
	std::set_intersection(mg1HeadWordSet.begin(), mg1HeadWordSet.end(), 
	                      mg2HeadWordSet.begin(), mg2HeadWordSet.end(),
						  std::inserter(intersectionSet, intersectionSet.begin()));

	if (intersectionSet.empty()) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by head word clash constraint";
		return true;
	}

	return false;
}
