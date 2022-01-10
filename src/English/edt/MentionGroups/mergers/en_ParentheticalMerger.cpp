// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_ParentheticalMerger.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/theories/Mention.h"

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

EnglishParentheticalMerger::EnglishParentheticalMerger(MentionGroupConstraint_ptr constraints) :
	PairwiseMentionGroupMerger(Symbol(L"EnglishParenthetical"), constraints) {}

bool EnglishParentheticalMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features = cache.getMentionPairFeaturesByName(m1, m2, Symbol(L"MentionPair-parenthetical"), Symbol(L"SingleParen"));
	BOOST_FOREACH(AttributeValuePair_ptr f, features) {
		boost::shared_ptr< AttributeValuePair<bool> > b;
		if ((b = boost::dynamic_pointer_cast< AttributeValuePair<bool> >(f))) {
			if (b->getValue()) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
						<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
						<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
					SessionLogger::dbg("MentionGroups_shouldMerge") << "SINGLE PARENTHETICAL FOUND";
				}
				return true;
			}
		}
	}
	return false;
}
