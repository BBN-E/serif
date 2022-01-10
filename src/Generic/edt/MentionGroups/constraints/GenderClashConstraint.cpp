// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/GenderClashConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Assert.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/Guesser.h"

bool GenderClashConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	std::set<Symbol> mg1Features = getGenderFeatures(mg1, cache);
	std::set<Symbol> mg2Features = getGenderFeatures(mg2, cache);

	bool m1_masc = (mg1Features.find(Guesser::MASCULINE) != mg1Features.end());
	bool m1_fem = (mg1Features.find(Guesser::FEMININE) != mg1Features.end());

	bool m2_masc = (mg2Features.find(Guesser::MASCULINE) != mg2Features.end());
	bool m2_fem = (mg2Features.find(Guesser::FEMININE) != mg2Features.end());

	// if results conflict within a single MentionGroup, don't trust our guesses
	if (m1_masc && m1_fem) return false;
	if (m2_masc && m2_fem) return false;

	if ((m1_masc && m2_fem) || (m1_fem && m2_masc)) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by gender clash constraint";
		return true;
	}

	return false;
}

std::set<Symbol> GenderClashConstraint::getGenderFeatures(const MentionGroup& group, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features;
	for (MentionGroup::const_iterator it = group.begin(); it != group.end(); ++it) {
		std::vector<AttributeValuePair_ptr> f = cache.getMentionFeaturesByName(*it, Symbol(L"Mention-gender"), Symbol(L"gender"));
		features.insert(features.end(), f.begin(), f.end());
	}
	return MentionGroupUtils::getSymbolValueSet(features);
}

/*
bool GenderClashConstraint::violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-gender"), Symbol(L"gender"));
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-gender"), Symbol(L"gender"));
	
	if (m1Features.empty() || m2Features.empty())
		return false;

	// check assumption there's only one gender feature per mention
	SerifAssert(m1Features.size() == 1);
	SerifAssert(m2Features.size() == 1);

	boost::shared_ptr< AttributeValuePair<Symbol> > m1GenderFeature = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(m1Features.front());
	boost::shared_ptr< AttributeValuePair<Symbol> > m2GenderFeature = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(m2Features.front());

	// check that casts didn't fail
	SerifAssert(m1GenderFeature != 0);
	SerifAssert(m2GenderFeature != 0);
	
	if (m1GenderFeature->getValue() == Guesser::UNKNOWN || m2GenderFeature->getValue() == Guesser::UNKNOWN)
		return false;

	if (m1GenderFeature->getValue() != m2GenderFeature->getValue()) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) 
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by gender clash constraint";
		return true;
	}
	return false;
}*/
