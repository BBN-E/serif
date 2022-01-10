// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/AcronymMerger.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

AcronymMerger::AcronymMerger(MentionGroupConstraint_ptr constraints) : PairwiseMentionGroupMerger(Symbol(L"Acronym"), constraints) {}

bool AcronymMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	// mentions must both be names
	if ((m1->getMentionType() != Mention::NAME && m1->getMentionType() != Mention::NEST) || 
		(m2->getMentionType() != Mention::NAME && m2->getMentionType() != Mention::NEST))
		return false;

	std::vector<AttributeValuePair_ptr> m1Acronyms = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-acronym"), Symbol(L"acronym"));
	std::vector<AttributeValuePair_ptr> m2Acronyms = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-acronym"), Symbol(L"acronym"));

	std::vector<AttributeValuePair_ptr> m1Names = cache.getMentionFeaturesByName(m1, Symbol(L"Mention-normalized-name"), Symbol(L"name"));
	std::vector<AttributeValuePair_ptr> m2Names = cache.getMentionFeaturesByName(m2, Symbol(L"Mention-normalized-name"), Symbol(L"name"));

	if (findNameAcronymMatch(m1Acronyms, m2Names) || findNameAcronymMatch(m2Acronyms, m1Names)) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
			SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
				<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
				<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
			SessionLogger::dbg("MentionGroups_shouldMerge") << "ACRONYM FOUND";	
		}
		return true;
	}

	return false;
}

bool AcronymMerger::findNameAcronymMatch(const std::vector<AttributeValuePair_ptr>& names, const std::vector<AttributeValuePair_ptr>& acronyms) const {
	BOOST_FOREACH(AttributeValuePair_ptr acronym, acronyms) {
		boost::shared_ptr< AttributeValuePair<Symbol> > a = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(acronym);
		BOOST_FOREACH(AttributeValuePair_ptr name, names) {
			boost::shared_ptr< AttributeValuePair<Symbol> > n = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(name);
			if (a->getValue() == n->getValue())
				return true;
		}
	}
	return false;
}
