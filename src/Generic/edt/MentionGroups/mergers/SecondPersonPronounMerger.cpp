// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/SecondPersonPronounMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

SecondPersonPronounMerger::SecondPersonPronounMerger(MentionGroupConstraint_ptr constraints) : 
	PairwiseMentionGroupMerger(Symbol(L"2ndPersonPronoun"), constraints) {}

bool SecondPersonPronounMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	// don't merge 2nd person pronouns if there could be multiple speakers in the document	
	if (CorefUtilities::hasSpeakerSourceType(cache.getDocTheory())) 
		return false;

	if (m1->getMentionType() == Mention::PRON && m2->getMentionType() == Mention::PRON) {
		Symbol headword1 = m1->getNode()->getHeadWord();
		Symbol headword2 = m2->getNode()->getHeadWord();
		if (WordConstants::is2pPronoun(headword1) && WordConstants::is2pPronoun(headword2)) {
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
				SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
					<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
					<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
				SessionLogger::dbg("MentionGroups_shouldMerge") << "2ND PERSON PRONOUNS";		
			}
			return true;
		}
	}
	return false;
}
