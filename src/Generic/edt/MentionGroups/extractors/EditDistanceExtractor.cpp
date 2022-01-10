// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/EditDistanceExtractor.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/edt/CorefUtilities.h"

#include <limits>
#include <boost/foreach.hpp>


EditDistanceExtractor::EditDistanceExtractor() : 
	AttributeValuePairExtractor<MentionPair>(Symbol(L"MentionPair"), Symbol(L"edit-distance")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EditDistanceExtractor::extractFeatures(const MentionPair& context,
                                                                           LinkInfoCache& cache,
                                                                           const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;

	if ((context.first->getMentionType() != Mention::NAME && context.first->getMentionType() != Mention::NEST) || 
		(context.second->getMentionType() != Mention::NAME && context.second->getMentionType() != Mention::NEST))
		return results;

	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(context.first, Symbol(L"Mention-normalized-name"), Symbol(L"name"));
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(context.second, Symbol(L"Mention-normalized-name"), Symbol(L"name"));

	float min_distance =  std::numeric_limits<float>::max();
	BOOST_FOREACH(AttributeValuePair_ptr feature1, m1Features) {
		boost::shared_ptr< AttributeValuePair<Symbol> > f1 = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(feature1);
		std::wstring m1 = f1->getValue().to_string();
		BOOST_FOREACH(AttributeValuePair_ptr feature2, m2Features) {
			boost::shared_ptr< AttributeValuePair<Symbol> > f2 = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(feature2);
			std::wstring m2 = f2->getValue().to_string();
			int e = CorefUtilities::editDistance(m1, m2);
			float d = (float)e/std::min(m1.length(), m2.length());
			if (d < min_distance)
				min_distance = d;
		}
	}

	results.push_back(AttributeValuePair<float>::create(Symbol(L"edit-distance"), min_distance, getFullName()));
	return results;
}
