// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/FirstPersonPronounToSpeakerMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"

FirstPersonPronounToSpeakerMerger::FirstPersonPronounToSpeakerMerger(MentionGroupConstraint_ptr constraints) : 
	PairwiseMentionGroupMerger(Symbol(L"1stPersonPronounToSpeaker"), constraints) {}

bool FirstPersonPronounToSpeakerMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (CorefUtilities::hasSpeakerSourceType(cache.getDocTheory())) {
		return (findFirstPersonReferentMatch(m1, m2, cache) || findFirstPersonReferentMatch(m2, m1, cache));
	}
	return false;
}

bool FirstPersonPronounToSpeakerMerger::findFirstPersonReferentMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	const DocTheory *docTheory = cache.getDocTheory();
	if (m2->getMentionType() == Mention::PRON && //m2->getEntityType().matchesPER() &&
		WordConstants::isSingular1pPronoun(m2->getNode()->getHeadWord()))
	{
		const MentionSet *mentionSet = m2->getMentionSet();
		for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
			if (docTheory->isSpeakerSentence(s)) {
				const MentionSet *speakerMS = docTheory->getSentenceTheory(s)->getMentionSet();
				if (speakerMS->getNMentions() >= 1) {
					const Mention *speakerMent = speakerMS->getMention(0);
					if (speakerMent->getEntityType().matchesPER() && speakerMent == m1) {
						if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
							SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
								<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
								<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
							SessionLogger::dbg("MentionGroups_shouldMerge") << "1ST PERSON PRONOUN TO SPEAKER";		
						}
						return true;
					} else {
						return false;
					}
				}
			}
		}
	}
	return false;
}
