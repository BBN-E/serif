// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_HighPrecisionPersonNameMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/SessionLogger.h"

EnglishHighPrecisionPersonNameMerger::EnglishHighPrecisionPersonNameMerger(MentionGroupConstraint_ptr constraints) :
	MentionGroupMerger(Symbol(L"EnglishHighPrecisionPersonName"), constraints) {}

bool EnglishHighPrecisionPersonNameMerger::shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	EntityType type1 = g1.getEntityType();
	EntityType type2 = g2.getEntityType();

	if (type1 != EntityType::getPERType() || type2 != EntityType::getPERType())
		return false;

	std::set<Symbol> g1Normalized = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g1, Symbol(L"Mention-normalized-name"), Symbol(L"name")));
	std::set<Symbol> g2Normalized = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g2, Symbol(L"Mention-normalized-name"), Symbol(L"name")));
	
	std::set<Symbol> g1Last = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g1, Symbol(L"Mention-person-name-variations"), Symbol(L"last")));
	std::set<Symbol> g2Last = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g2, Symbol(L"Mention-person-name-variations"), Symbol(L"last")));

	std::set<Symbol> g1First = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g1, Symbol(L"Mention-person-name-variations"), Symbol(L"first")));
	std::set<Symbol> g2First = MentionGroupUtils::getSymbolValueSet(cache.getMentionFeaturesByName(g2, Symbol(L"Mention-person-name-variations"), Symbol(L"first")));

	
	if (findUniqueValueMatch(g1, g2, g1Normalized, g2Last, Symbol(L"Mention-person-name-variations"), Symbol(L"last"), cache) ||
		findUniqueValueMatch(g1, g2, g2Normalized, g1Last, Symbol(L"Mention-person-name-variations"), Symbol(L"last"), cache) ||
		findUniqueValueMatch(g1, g2, g1Normalized, g2First, Symbol(L"Mention-person-name-variations"), Symbol(L"first"), cache) ||
		findUniqueValueMatch(g1, g2, g2Normalized, g1First, Symbol(L"Mention-person-name-variations"), Symbol(L"first"), cache))
	{
		return true;
	}
	return false;
}

bool EnglishHighPrecisionPersonNameMerger::findUniqueValueMatch(const MentionGroup& g1, const MentionGroup& g2,
                                                                std::set<Symbol>& nameFeatureSet, std::set<Symbol>& uniqFeatureSet,
                                                                Symbol extractor, Symbol feature, LinkInfoCache& cache) const
{
	for (std::set<Symbol>::iterator it1 = nameFeatureSet.begin(); it1 != nameFeatureSet.end(); ++it1) {
		for (std::set<Symbol>::iterator it2 = uniqFeatureSet.begin(); it2 != uniqFeatureSet.end(); ++it2) {
			if ((*it1) == (*it2) && MentionGroupUtils::featureIsUniqueInDoc(extractor, feature, *it2, g1, g2, cache)) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					const Mention *f1 = *(g1.begin());
					const Mention *f2 = *(g2.begin());
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " << f1->getUID() << " and " << f2->getUID();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "\tfeature1: " << (*it1).to_string() 
																  << " feature2: " << (*it2).to_string();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "HIGH PRECISION PERSON NAME MATCH";			
				}	
				return true;
			}
		}
	}
	return false;
}
