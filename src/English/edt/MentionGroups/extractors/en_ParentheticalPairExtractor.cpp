// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/SynNode.h"

#include "English/edt/MentionGroups/extractors/en_ParentheticalPairExtractor.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

#include <boost/foreach.hpp>

EnglishParentheticalPairExtractor::EnglishParentheticalPairExtractor() : 
	AttributeValuePairExtractor<MentionPair>(Symbol(L"MentionPair"), Symbol(L"parenthetical")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishParentheticalPairExtractor::extractFeatures(const MentionPair& context, 
																			  LinkInfoCache& cache, 
																			  const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;
	
	const Mention *m1 = context.first;
	const Mention *m2 = context.second;

	// Only examine special parenthetical mentions that occur right after the other mention, or the last contained part of the other mention
	if (m1->getSentenceNumber() == m2->getSentenceNumber() && m2->getEntityType() == Symbol(L"POG")) {
		if (m1->getNode() &&
			m2->getNode() &&
			(m2->getNode()->getStartToken() == m1->getNode()->getEndToken() + 2 ||
			 (m2->getNode()->getParent() &&
			  m2->getNode()->getParent()->getParent() == m1->getNode() &&
			  m2->getNode()->getEndToken() + 1 == m1->getNode()->getEndToken()))) {
			results.push_back(AttributeValuePair<bool>::create(m2->getEntitySubtype().getName(), true, getFullName()));
		}
	}

	return results;
}
