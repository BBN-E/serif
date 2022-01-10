// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/NumberClashConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Assert.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/Guesser.h"

bool NumberClashConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	std::set<Symbol> mg1Features = getNumberFeatures(mg1, cache);
	std::set<Symbol> mg2Features = getNumberFeatures(mg2, cache);

	bool m1_singular = (mg1Features.find(Guesser::SINGULAR) != mg1Features.end());
	bool m1_plural = (mg1Features.find(Guesser::PLURAL) != mg1Features.end());

	bool m2_singular = (mg2Features.find(Guesser::SINGULAR) != mg2Features.end());
	bool m2_plural = (mg2Features.find(Guesser::PLURAL) != mg2Features.end());

	// if results conflict within a single MentionGroup, don't trust our guesses
	if (m1_singular && m1_plural) return false;
	if (m2_singular && m2_plural) return false;

	if ((m1_singular && m2_plural) || (m1_plural && m2_singular)) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by number clash constraint";
		return true;
	}

	return false;
}

std::set<Symbol> NumberClashConstraint::getNumberFeatures(const MentionGroup& group, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features;
	for (MentionGroup::const_iterator it = group.begin(); it != group.end(); ++it) {
		std::vector<AttributeValuePair_ptr> f = cache.getMentionFeaturesByName(*it, Symbol(L"Mention-number"), Symbol(L"number"));
		features.insert(features.end(), f.begin(), f.end());
	}
	return MentionGroupUtils::getSymbolValueSet(features);
}

/*
bool NumberClashConstraint::violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-number"), Symbol(L"number"));
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-number"), Symbol(L"number"));
	
	if (m1Features.empty() || m2Features.empty())
		return false;

	// check assumption there's only one number feature per mention
	SerifAssert(m1Features.size() == 1);
	SerifAssert(m2Features.size() == 1);

	boost::shared_ptr< AttributeValuePair<Symbol> > m1NumberFeature = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(m1Features.front());
	boost::shared_ptr< AttributeValuePair<Symbol> > m2NumberFeature = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(m2Features.front());

	// check that casts didn't fail
	SerifAssert(m1NumberFeature != 0);
	SerifAssert(m2NumberFeature != 0);
	
	if (m1NumberFeature->getValue() == Guesser::UNKNOWN || m2NumberFeature->getValue() == Guesser::UNKNOWN)
		return false;

	if (m1NumberFeature->getValue() != m2NumberFeature->getValue()) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by number clash constraint";
		return true;
	}
	return false;
}*/
