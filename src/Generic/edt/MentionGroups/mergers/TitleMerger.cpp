// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/TitleMerger.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

TitleMerger::TitleMerger(MentionGroupConstraint_ptr constraints) 
	: PairwiseMentionGroupMerger(Symbol(L"Title"), constraints) {}

bool TitleMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	return (findTitleMatch(m1, m2, cache) || findTitleMatch(m2, m1, cache));
}

bool TitleMerger::findTitleMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	// Compare all title attributes of m2 with value of m1, return true if match found.
	std::vector<AttributeValuePair_ptr> features = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-title"), Symbol(L"title"));
	BOOST_FOREACH(AttributeValuePair_ptr f, features) {
		boost::shared_ptr< AttributeValuePair<const Mention*> > title;
		if ((title = boost::dynamic_pointer_cast< AttributeValuePair<const Mention*> >(f))) {
			if (title->getValue()->getUID() == m1->getUID()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
						<< m1->getUID() << " (" << m1->toCasedTextString() << ") and "	
						<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
					SessionLogger::dbg("MentionGroups_shouldMerge") << "TITLE FOUND";			
				}
				return true;
			}
		}
	}
	return false;
}
