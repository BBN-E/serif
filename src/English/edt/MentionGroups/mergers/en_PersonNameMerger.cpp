// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_PersonNameMerger.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/theories/Mention.h"

EnglishPersonNameMerger::EnglishPersonNameMerger(MentionGroupConstraint_ptr constraints) :
	PairwiseMentionGroupMerger(Symbol(L"EnglishPersonName"), constraints) {}

bool EnglishPersonNameMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	
	// Only examine pairs of PER NAME mentions
	Mention::Type m1Type = m1->getMentionType();
	Mention::Type m2Type = m2->getMentionType();
	if (m1->getEntityType() != EntityType::getPERType() || (m1Type != Mention::NAME && m1Type != Mention::NEST) || 
		m2->getEntityType() != EntityType::getPERType() || (m2Type != Mention::NAME && m2Type != Mention::NEST))
	{
		return false;
	}

	std::vector<AttributeValuePair_ptr> m1Normalized = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-normalized-name"), Symbol(L"name"));
	std::vector<AttributeValuePair_ptr> m2Normalized = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-normalized-name"), Symbol(L"name"));

	std::vector<AttributeValuePair_ptr> m1Last = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-person-name-variations"), Symbol(L"last"));
	std::vector<AttributeValuePair_ptr> m2Last = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-person-name-variations"), Symbol(L"last"));

	std::vector<AttributeValuePair_ptr> m1First = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-person-name-variations"), Symbol(L"first"));
	std::vector<AttributeValuePair_ptr> m2First = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-person-name-variations"), Symbol(L"first"));
	
	// Look for a match between the full normalized name of one Mention and either the first or last name of the other.
	if (findValueMatch(m1, m2, m1Normalized, m2Last) || findValueMatch(m1, m2, m1Last, m2Normalized) ||
		findValueMatch(m1, m2, m1Normalized, m2First) || findValueMatch(m1, m2, m1First, m2Normalized)) 
	{
		return true;
	}


	return false;
}

bool EnglishPersonNameMerger::findValueMatch(const Mention *m1, const Mention *m2, 
                                             std::vector<AttributeValuePair_ptr> featureSet1,
                                             std::vector<AttributeValuePair_ptr> featureSet2) const 
{
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = featureSet1.begin(); it1 != featureSet1.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = featureSet2.begin(); it2 != featureSet2.end(); ++it2) {
			if ((*it1)->valueEquals(*it2)) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " << m1->getUID() << " and " << m2->getUID();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "\tfeature1: " << (*it1)->toString() 
																  << " feature2: " << (*it2)->toString();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "PERSON NAME MATCH";			
				}	
				return true;
			}
		}
	}
	return false;
}
