// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/OperatorClashConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"

#include <boost/foreach.hpp>

OperatorClashConstraint::OperatorClashConstraint() {
	_operatorKeys.insert(Symbol(L"email"));
	_operatorKeys.insert(Symbol(L"ip"));
	_operatorKeys.insert(Symbol(L"phone"));
}

bool OperatorClashConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	BOOST_FOREACH(Symbol operatorKey, _operatorKeys) {
		std::set<Symbol> mg1operators = getOperatorFeatures(operatorKey, mg1, cache);
		std::set<Symbol> mg2operators = getOperatorFeatures(operatorKey, mg2, cache);
		if (!(std::includes(mg1operators.begin(), mg1operators.end(), mg2operators.begin(), mg2operators.end()) ||
			  std::includes(mg2operators.begin(), mg2operators.end(), mg1operators.begin(), mg1operators.end()))) {
			// Constraint violated if there are conflicting operator values (i.e. one has to be a subset of the other)
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
				SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by " << operatorKey.to_debug_string() << " operator clash constraint";
			return true;
		}
	}

	return false;
}

std::set<Symbol> OperatorClashConstraint::getOperatorFeatures(Symbol& operatorKey, const MentionGroup& group, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features;
	for (MentionGroup::const_iterator it = group.begin(); it != group.end(); ++it) {
		std::vector<AttributeValuePair_ptr> f = cache.getMentionFeaturesByName(*it, Symbol(L"Mention-operator"), operatorKey);
		features.insert(features.end(), f.begin(), f.end());
	}
	return MentionGroupUtils::getSymbolValueSet(features);
}
