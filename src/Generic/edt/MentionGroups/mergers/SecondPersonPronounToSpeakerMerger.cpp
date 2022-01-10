// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/SecondPersonPronounToSpeakerMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"

SecondPersonPronounToSpeakerMerger::SecondPersonPronounToSpeakerMerger(MentionGroupConstraint_ptr constraints) 
	: PairwiseMentionGroupMerger(Symbol(L"2ndPersonPronounToSpeaker"), constraints) {}

bool SecondPersonPronounToSpeakerMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (CorefUtilities::hasSpeakerSourceType(cache.getDocTheory())) {
		return (findSecondPersonReferentMatch(m1, m2, cache) || findSecondPersonReferentMatch(m2, m1, cache));
	}
	return false;
}

bool SecondPersonPronounToSpeakerMerger::findSecondPersonReferentMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	const DocTheory *docTheory = cache.getDocTheory();
	if (m2->getMentionType() == Mention::PRON && WordConstants::is2pPronoun(m2->getNode()->getHeadWord())) {
		const MentionSet *mentionSet = m2->getMentionSet();
		bool already_found_first_speaker = false;
		for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
			if (docTheory->isSpeakerSentence(s)) {
				const MentionSet *speakerMS = docTheory->getSentenceTheory(s)->getMentionSet();
				if (speakerMS->getNMentions() >= 1) {
					const Mention *speakerMent = speakerMS->getMention(0);
					if (already_found_first_speaker) {
						if (speakerMent->getEntityType().matchesPER() && speakerMent == m1) {
							if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
								SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
									<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
									<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
								SessionLogger::dbg("MentionGroups_shouldMerge") << "2ND PERSON PRONOUN TO SPEAKER";		
							}
							return true;
						} else {
							break; // continue looking for receiver sentences
						}
					}
					already_found_first_speaker = true;
				}
			}
			if (docTheory->isReceiverSentence(s)) {
				const MentionSet *receiverMS = docTheory->getSentenceTheory(s)->getMentionSet();
				if (receiverMS->getNMentions() >= 1) {
					const Mention *receiverMent = receiverMS->getMention(0);
					if (receiverMent->getEntityType().matchesPER() && receiverMent == m1) {
						if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
							SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
								<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
								<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
							SessionLogger::dbg("MentionGroups_shouldMerge") << "2ND PERSON PRONOUN TO RECEIVER";		
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
