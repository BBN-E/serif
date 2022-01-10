// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_PersonNamePairExtractor.h"
#include "Generic/edt/MentionGroups/extractors/EnglishPersonNameVariationsExtractor.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

#include <boost/foreach.hpp>

EnglishPersonNamePairExtractor::EnglishPersonNamePairExtractor() : 
	AttributeValuePairExtractor<MentionPair>(Symbol(L"MentionPair"), Symbol(L"person-name")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishPersonNamePairExtractor::extractFeatures(const MentionPair& context, 
																			  LinkInfoCache& cache, 
																			  const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;
	
	const Mention *m1 = context.first;
	const Mention *m2 = context.second;

	// Only examine pairs of PER NAME mentions
	Mention::Type m1Type = m1->getMentionType();
	Mention::Type m2Type = m2->getMentionType();
	if (m1->getEntityType() != EntityType::getPERType() || (m1Type != Mention::NAME && m1Type != Mention::NEST) || 
		m2->getEntityType() != EntityType::getPERType() || (m2Type != Mention::NAME && m2Type != Mention::NEST))
	{
		return results;
	}


	std::vector<Symbol> allNameFeatures;
	allNameFeatures.push_back(EnglishPersonNameVariationsExtractor::FIRST);
	allNameFeatures.push_back(EnglishPersonNameVariationsExtractor::MIDDLE);
	allNameFeatures.push_back(EnglishPersonNameVariationsExtractor::MIDDLE_TOKEN);
	allNameFeatures.push_back(EnglishPersonNameVariationsExtractor::LAST);
	addMatchAndClashFeatures(results, cache, m1, m2, Symbol(L"Mention-person-name-variations"), EnglishPersonNameVariationsExtractor::LAST, allNameFeatures);
	addMatchAndClashFeatures(results, cache, m1, m2, Symbol(L"Mention-person-name-variations"), EnglishPersonNameVariationsExtractor::FIRST, allNameFeatures);
	addMatchAndClashFeatures(results, cache, m1, m2, Symbol(L"Mention-person-name-variations"), EnglishPersonNameVariationsExtractor::SUFFIX, allNameFeatures);

	return results;
}

void EnglishPersonNamePairExtractor::addMatchAndClashFeatures(std::vector<AttributeValuePair_ptr>& results, 
															  LinkInfoCache& cache,
															  const Mention *m1, const Mention *m2,
															  Symbol extractorName, Symbol featureName, 
															  std::vector<Symbol> &allNameFeatures) 
{
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, extractorName, featureName);
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, extractorName, featureName);

	std::vector<AttributeValuePair_ptr> m1AdditionalFeatures;
	std::vector<AttributeValuePair_ptr> m2AdditionalFeatures;
	for (std::vector<Symbol>::iterator it = allNameFeatures.begin(); it != allNameFeatures.end(); ++it) {
		if (*it == featureName)
			continue;
		std::vector<AttributeValuePair_ptr> m1OtherNameFeatures = cache.getMentionFeaturesByName(m1, extractorName, (*it));
		std::vector<AttributeValuePair_ptr> m2OtherNameFeatures = cache.getMentionFeaturesByName(m2, extractorName, (*it));
		m1AdditionalFeatures.insert(m1AdditionalFeatures.end(), m1OtherNameFeatures.begin(), m1OtherNameFeatures.end());
		m2AdditionalFeatures.insert(m2AdditionalFeatures.end(), m2OtherNameFeatures.begin(), m2OtherNameFeatures.end());
	}	

	if (m1Features.empty() || m2Features.empty()) 
		return;

	bool match = false;
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = m1Features.begin(); it1 != m1Features.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = m2Features.begin(); it2 != m2Features.end(); ++it2) {
			if ((*it1)->equals(*it2))
				match = true;
		}
	}

	bool wrong_name_match = false;
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = m1Features.begin(); it1 != m1Features.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = m2AdditionalFeatures.begin(); it2 != m2AdditionalFeatures.end(); ++it2) {
			if ((*it1)->valueEquals(*it2))
				wrong_name_match = true;
		}
	}
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = m2Features.begin(); it1 != m2Features.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = m1AdditionalFeatures.begin(); it2 != m1AdditionalFeatures.end(); ++it2) {
			if ((*it1)->valueEquals(*it2))
				wrong_name_match = true;
		}
	}

	if (match) {
		std::wstring match_key = featureName.to_string() + std::wstring(L"-match");			
		results.push_back(AttributeValuePair<bool>::create(Symbol(match_key), true, getFullName()));
	} else if (!wrong_name_match) {
		std::wstring clash_key = featureName.to_string() + std::wstring(L"-clash");	
		results.push_back(AttributeValuePair<bool>::create(Symbol(clash_key), true, getFullName()));
	}
}
