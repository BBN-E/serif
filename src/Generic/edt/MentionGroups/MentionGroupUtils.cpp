// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/common/Assert.h"
#include "Generic/common/SessionLogger.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>  

std::set<Symbol> MentionGroupUtils::getSymbolValueSet(std::vector<AttributeValuePair_ptr> pairs) {
	std::set<Symbol> result;
	BOOST_FOREACH(AttributeValuePair_ptr pair, pairs) {
		boost::shared_ptr< AttributeValuePair<Symbol> > symPair = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(pair);
		// check that cast didn't fail
		SerifAssert(symPair != 0);
		result.insert(symPair->getValue());
	}
	return result;
}

std::set<int> MentionGroupUtils::getIntValueSet(std::vector<AttributeValuePair_ptr> pairs) {
	std::set<int> result;
	BOOST_FOREACH(AttributeValuePair_ptr pair, pairs) {
		boost::shared_ptr< AttributeValuePair<int> > symIntPair = boost::dynamic_pointer_cast< AttributeValuePair<int> >(pair);
		// check that cast didn't fail
		SerifAssert(symIntPair != 0);
		result.insert(symIntPair->getValue());
	}
	return result;
}


void MentionGroupUtils::addSymbolFeatures(std::vector<AttributeValuePair_ptr>& features,
                                          Symbol extractorName, Symbol featureName,
                                          std::wstring value, LinkInfoCache& cache) 
{
	// normalize the feature value
	std::transform(value.begin(), value.end(), value.begin(), towlower);
	boost::replace_all(value, L".", L"");
	Symbol normalizedVal = Symbol(value.c_str());
	features.push_back(AttributeValuePair<Symbol>::create(featureName, normalizedVal, extractorName));
	
	// check for alternate spellings
	Symbol altValue = cache.lookupAlternateSpelling(normalizedVal);
	if (!altValue.is_null()) {
		features.push_back(AttributeValuePair<Symbol>::create(featureName, altValue, extractorName));
	}
}

bool MentionGroupUtils::featureIsUniqueInDoc(Symbol extractorName, Symbol featureName, Symbol value,
                                             const MentionGroup& g1, const MentionGroup& g2,
                                             LinkInfoCache& cache)
{
	std::vector<const Mention*> matches = cache.getMentionsByFeatureValue(extractorName, featureName, value);
	BOOST_FOREACH(const Mention* m, matches) {
		if (!g1.contains(m) && !g2.contains(m)) {
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge"))
				SessionLogger::dbg("MentionGroups_shouldMerge") 
					<< "    Uniqueness of " << g1.toString() << " AND " 
					<< g2.toString() << " blocked by " << m->getUID();
			return false;
		}
	}

	return true;
}
