// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/mergers/en_RelativePronounMerger.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "English/common/en_WordConstants.h"
#include "English/edt/en_PreLinker.h"

EnglishRelativePronounMerger::EnglishRelativePronounMerger(MentionGroupConstraint_ptr constraints) :
	PairwiseMentionGroupMerger(Symbol(L"EnglishRelativePronoun"), constraints) {}

bool EnglishRelativePronounMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (m1->getMentionType() != Mention::PRON && m2->getMentionType() != Mention::PRON)
		return false;

	std::vector<AttributeValuePair_ptr> features = cache.getMentionPairFeaturesByName(m1, m2, Symbol(L"MentionPair-whq-link"), Symbol(L"whq-link"));

	if (features.empty())
		return false;

	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
		SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
			<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
			<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
		SessionLogger::dbg("MentionGroups_shouldMerge") << "WHQ LINK FOUND";
	}
	return true;
}
