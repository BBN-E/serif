// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_NationalityPeopleMerger.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/theories/Mention.h"
#include "English/edt/en_PreLinker.h"

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

EnglishNationalityPeopleMerger::EnglishNationalityPeopleMerger(MentionGroupConstraint_ptr constraints) :
	PairwiseMentionGroupMerger(Symbol(L"EnglishNationalityPeople"), constraints) {}

bool EnglishNationalityPeopleMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features = cache.getMentionPairFeaturesByName(m1, m2, Symbol(L"MentionPair-shared-proposition"), Symbol(L"shared-proposition"));
	BOOST_FOREACH(AttributeValuePair_ptr f, features) {
		boost::shared_ptr< AttributeValuePair<const Proposition*> > p;
		if ((p = boost::dynamic_pointer_cast< AttributeValuePair<const Proposition*> >(f))) {
			const Proposition *prop = p->getValue();
			const MentionSet *mentionSet = m1->getMentionSet();
			PreLinker::MentionMap links;
			EnglishPreLinker::preLinkNationalityPeople(links, prop, mentionSet);
			if (links[m1->getIndex()] == m2 ||
				links[m2->getIndex()] == m1)
			{
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
						<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
						<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
					SessionLogger::dbg("MentionGroups_shouldMerge") << "NATIONALITY PEOPLE FOUND";
				}
				return true;
			}
		}
	}
	return false;
}