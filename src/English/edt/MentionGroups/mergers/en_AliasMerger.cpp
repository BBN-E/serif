// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_AliasMerger.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

EnglishAliasMerger::EnglishAliasMerger(MentionGroupConstraint_ptr constraints) 
	: PairwiseMentionGroupMerger(Symbol(L"EnglishAlias"), constraints) {}

bool EnglishAliasMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-alias"), Symbol(L"alias"));
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-alias"), Symbol(L"alias"));
	if (findMentionFeatureMatch(m1, m2Features) || findMentionFeatureMatch(m2, m1Features)) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
			SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
				<< m1->getUID() << " (" << m1->toCasedTextString() << ") and "	
				<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
			SessionLogger::dbg("MentionGroups_shouldMerge") << "ALIAS FOUND";			
		}
		return true;
	}
	return false;
}

bool EnglishAliasMerger::findMentionFeatureMatch(const Mention *ment, std::vector<AttributeValuePair_ptr>& features) const {
	BOOST_FOREACH(AttributeValuePair_ptr f, features) {
		boost::shared_ptr< AttributeValuePair<const Mention*> > m;
		if ((m = boost::dynamic_pointer_cast< AttributeValuePair<const Mention*> >(f))) {
			if (m->getValue()->getUID() == ment->getUID()) {
				return true;
			}
		}
	}
	return false;
}
