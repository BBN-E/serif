// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Chinese/events/ch_EventLinker.h"
#include "Generic/events/EventFinder.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Chinese/common/ch_WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Chinese/parse/ch_STags.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

ChineseEventLinker::ChineseEventLinker(): _use_correct_answers(false) {
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

void ChineseEventLinker::linkEvents(const EventMentionSet *eventMentionSet, 
							 EventSet *eventSet,
							 const EntitySet *entitySet,
							 const PropositionSet *propSet,
							 CorrectAnswers *correctAnswers,
							 int sentence_num,
							 Symbol docname) 
{

	// sort them -- very clumsy, but I don't really care at the moment,
	//  there should never be more than a couple per sentence
	EventMention *mentions[MAX_SENTENCE_EVENTS];
	bool already_used[MAX_SENTENCE_EVENTS];
	for (int ment = 0; ment < eventMentionSet->getNEventMentions(); ment++) {
		already_used[ment] = false;
	}
	int n_mentions = 0;
	while (true) {
		int earliest_token = 10000;
		int earliest_index = -1;
		for (int i = 0; i < eventMentionSet->getNEventMentions(); i++) {
			EventMention *em = eventMentionSet->getEventMention(i);
			if (already_used[i])
				continue;
			if (em->getAnchorNode()->getStartToken() < earliest_token) {
				earliest_token = em->getAnchorNode()->getStartToken();
				earliest_index = i;
			}
		}
		if (earliest_index == -1)
			break;
		else {
			mentions[n_mentions] = eventMentionSet->getEventMention(earliest_index);
			already_used[earliest_index] = true;
			n_mentions++;
		}
	}

	for (int i = 0; i < n_mentions; i++) {
		if (mentions[i] == 0)
			continue;

		// fill in candidates -- most recent to least recent
		_candidates.clear();
		for (int j = eventSet->getNEvents() - 1; j >= 0; j--) {
			Event *e = eventSet->getEvent(j);
			if (e->getType() == mentions[i]->getEventType()) {
				_candidates.push_back(e);
			}
		}

		Event *e = 0;

		Symbol thisEventID;
		if (_use_correct_answers) {
			thisEventID = correctAnswers->getEventID(mentions[i], docname);
			for (int cand = 0; cand < (int) _candidates.size(); cand++) {
				if (thisEventID == _candidates[cand]->getAnnotationID())
					e = _candidates[cand];
			}
		} else {
			e = findBestCandidate(mentions[i], entitySet, propSet);
		}

		if (e == 0) {
			e = _new Event(eventSet->getNEvents());
			eventSet->takeEvent(e);
			if (_use_correct_answers) {
				e->setAnnotationID(thisEventID);
			}
		} else {
			if (EventFinder::DEBUG) {
				EventFinder::_debugStream << "Linking " << mentions[i]->toString();
				EventFinder::_debugStream << "to " << e->toString();
			}
		}

		e->addEventMention(mentions[i]);
		
	}

}

Event *ChineseEventLinker::findBestCandidate(EventMention *em, const EntitySet *entitySet, 
									  const PropositionSet *propSet) 
{

	if (_candidates.size() == 0)
		return 0;
	
	// NB: candidates are already in order of most to least recent

/*	// if victims are equal
	const Mention *victim = em->getFirstMentionForSlot(victimSym);
	if (victim != 0) {
		for (int cand = 0; cand < (int) _candidates.size(); cand++) {
			const Mention *candVictim = _candidates[cand]->getFirstMentionForSlot(victimSym);
			if (candVictim != 0) {
				Entity *ourEntity = entitySet->getEntityByMention(victim->getUID());
				Entity *candEntity = entitySet->getEntityByMention(candVictim->getUID());
				if (ourEntity == candEntity)
					return _candidates[cand];
			}
		}
	}

	// if objectActedOns are equal
	const Mention *objectActedOn = em->getFirstMentionForSlot(objectActedOnSym);
	if (objectActedOn != 0) {
		for (int cand = 0; cand < (int) _candidates.size(); cand++) {
			const Mention *candOAO = _candidates[cand]->getFirstMentionForSlot(objectActedOnSym);
			if (candOAO != 0) {
				Entity *ourEntity = entitySet->getEntityByMention(objectActedOn->getUID());
				Entity *candEntity = entitySet->getEntityByMention(candOAO->getUID());
				if (ourEntity == candEntity)
					return _candidates[cand];
			}
		}
	}

	if (em->getAnchorProp() != 0 && 
		em->getAnchorProp()->getPredType() == Proposition::NOUN_PRED) {
		int this_event = em->getAnchorProp()->getArg(0)->getMentionIndex();
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			Proposition *prop = propSet->getProposition(i);
			if (prop->getPredType() == Proposition::COPULA_PRED) {
				int subject = -1;
				int object = -1;
				for (int j = 0; j < prop->getNArgs(); j++) {
					if (prop->getArg(j)->getType() == Argument::MENTION_ARG) {
						if (prop->getArg(j)->getRoleSym() == Argument::SUB_ROLE)
							subject = prop->getArg(j)->getMentionIndex();
						else if (prop->getArg(j)->getRoleSym() == Argument::OBJ_ROLE)
							object = prop->getArg(j)->getMentionIndex();
					}
				}
				if (subject < 0 || object < 0)
					continue;
				int to_match = -1;
				if (this_event == subject)
					to_match = object;
				else if (this_event == object)
					to_match = subject;
				if (to_match < 0)
					continue;
				for (int cand = 0; cand < (int) _candidates.size(); cand++) {
					Event::LinkedEventMention *lem = _candidates[cand]->getEventMentions();
					while (lem != 0) {
						const Proposition *anchor = lem->eventMention->getAnchorProp();
						if (anchor != 0 &&
							anchor->getPredType() == Proposition::NOUN_PRED &&
							anchor->getArg(0)->getMentionIndex() == to_match) 
						{
							return _candidates[cand];
						}
						lem = lem->next;
					}
				}
			}
		}
	}
*/			
/*
	// this is kind of a wash, so I'm going to turn it off
	// if BOTH performedBy and toLocation are equal
	const Mention *performedBy = em->getMentionForSlot(performedBySym);
	const Mention *toLocation = em->getMentionForSlot(toLocationSym);
	if (performedBy != 0 && toLocation != 0) {
		for (int cand = 0; cand < (int) _candidates.size(); cand++) {
			const Mention *candPB = _candidates[cand]->getMentionForSlot(performedBySym);
			const Mention *candTL = _candidates[cand]->getMentionForSlot(toLocationSym);			
			if (candPB != 0 && candTL != 0) {
				Entity *ourEntity = entitySet->getEntityByMention(performedBy->getUID());
				Entity *candEntity = entitySet->getEntityByMention(candPB->getUID());
				if (ourEntity == candEntity) {
					ourEntity = entitySet->getEntityByMention(toLocation->getUID());
					candEntity = entitySet->getEntityByMention(candTL->getUID());
					if (ourEntity == candEntity) {
						return _candidates[cand];
					}
				}
			}
		}
	}

	// if BOTH performedBy and fromLocation are equal
	const Mention *fromLocation = em->getMentionForSlot(fromLocationSym);
	if (performedBy != 0 && fromLocation != 0) {
		for (int cand = 0; cand < (int) _candidates.size(); cand++) {
			const Mention *candPB = _candidates[cand]->getMentionForSlot(performedBySym);
			const Mention *candFL = _candidates[cand]->getMentionForSlot(fromLocationSym);			
			if (candPB != 0 && candFL != 0) {
				Entity *ourEntity = entitySet->getEntityByMention(performedBy->getUID());
				Entity *candEntity = entitySet->getEntityByMention(candPB->getUID());
				if (ourEntity == candEntity) {
					ourEntity = entitySet->getEntityByMention(fromLocation->getUID());
					candEntity = entitySet->getEntityByMention(candFL->getUID());
					if (ourEntity == candEntity) {
						return _candidates[cand];
					}
				}
			}
		}
	}
*/
	/*
	// link events that share >= 1 relation and no contradictory ones...?
	// ...no, let's not, since it doesn't work...
	int nslots = em->getNumSlots();
	for (int cand = 0; cand < (int) _candidates.size(); cand++) {
		bool matched = false;
		for (int i = 0; i < nslots; i++) {
			Entity *ourEntity = entitySet->getEntityByMention(em->getNthSlotMention(i)->getUID());
			const Mention *candMention 
				= _candidates[cand]->getMentionForSlot(em->getNthSlotName(i));
			if (candMention == 0)
				continue;
			if (entitySet->getEntityByMention(candMention->getUID()) != ourEntity)
				break;
			matched = true;
		}
		if (matched)
			return _candidates[cand];
	}*/
	
	return 0;
}

// look for a definite article in the first premod of the node
bool ChineseEventLinker::anchorIsDefiniteNounPhrase(EventMention *em)
{
/*	if (em->getAnchorProp() == 0)
		return false;
	if (em->getAnchorProp()->getPredType() != Proposition::NOUN_PRED)
		return false;
	const SynNode *npa = em->getAnchorNode()->getHeadPreterm()->getParent();
	if (npa != 0) {
		const SynNode* pre = npa->getChild(0);
		// if no premod, not definite
		if (pre == 0 || pre == npa->getHead())
			return false;
		if (pre->getTag() == ChineseSTags::DT) {
			Symbol preWord = pre->getHeadWord();
			if (preWord == ChineseWordConstants::THE ||
				preWord == ChineseWordConstants::THAT ||
				preWord == ChineseWordConstants::THIS ||
				preWord == ChineseWordConstants::THESE ||
				preWord == ChineseWordConstants::THOSE)
				return true;
		}
	}
*/
	return false;
}
