// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "English/values/en_DeprecatedEventValueRecognizer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/events/patterns/EventPatternMatcher.h"
#include "Generic/events/patterns/EventPatternSet.h"
#include "Generic/events/patterns/EventSearchNode.h"

EnglishDeprecatedEventValueRecognizer::EnglishDeprecatedEventValueRecognizer() {

	std::string rules = ParamReader::getParam("event_value_patterns");
	std::string type = ParamReader::getParam("event_model_type");

	if (!rules.empty() && !type.empty() && type != "NONE") {
		EventPatternSet *set = _new EventPatternSet(rules.c_str());
		_matcher = _new EventPatternMatcher(set);	
	} else 
		_matcher = 0;
	
	TOPLEVEL_PATTERNS = Symbol(L"TOPLEVEL_PATTERNS");
	CERTAIN_SENTENCES = Symbol(L"CERTAIN_SENTENCES");

}

void EnglishDeprecatedEventValueRecognizer::createEventValues(DocTheory* docTheory) {
	_n_value_mentions = 0;

	EventSet *eventSet = docTheory->getEventSet();

	if (eventSet != 0) {
		for (int i = 0; i < eventSet->getNEvents(); i++) {
			Event *event = eventSet->getEvent(i);
			Event::LinkedEventMention *vment = event->getEventMentions();
			while (vment != 0) {
				int sentno = vment->eventMention->getSentenceNumber();			
				const PropositionSet *propSet = docTheory->getSentenceTheory(sentno)->getPropositionSet();

				if (_matcher != 0) {
					_matcher->resetForNewSentence(propSet,
						docTheory->getSentenceTheory(sentno)->getMentionSet(),
						docTheory->getSentenceTheory(sentno)->getValueMentionSet());
				}

				bool found_position = false;
				bool found_sentence = false;
				if (vment->eventMention->getAnchorProp() != 0 && _matcher != 0) {

					EventSearchNode *result = 
						_matcher->matchAllPossibleNodes(vment->eventMention->getAnchorProp(), 
														TOPLEVEL_PATTERNS);
					if (result != 0) {
						EventSearchNode *iter = result;
						EventSearchNode *iterNext;
						while (iter != 0) {
							iterNext = iter->getNext();
							iter->setNext(0);
							if (getBaseType(vment->eventMention->getEventType()) == Symbol(L"Justice") &&
								vment->eventMention->getFirstValueForSlot(Symbol(L"Crime")) == 0) 
							{
								const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Crime"));
								if (node) {
									
									addValueMention(docTheory, vment->eventMention, node, 
										Symbol(L"Crime"), Symbol(L"Crime"), 1.0);
								}
							}
							if (vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
								vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0) 
							{
								const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Sentence"));
								if (node) {
									addValueMention(docTheory, vment->eventMention, node, 
										Symbol(L"Sentence"), Symbol(L"Sentence"), 1.0);
								}
							}						
							if (getBaseType(vment->eventMention->getEventType()) == Symbol(L"Personnel") &&
								vment->eventMention->getFirstValueForSlot(Symbol(L"Position")) == 0) 
							{
								const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Position"));
								if (node) {
									addValueMention(docTheory, vment->eventMention, node, 
										Symbol(L"Job-Title"), Symbol(L"Position"), 1.0);
									found_position = true;
								}
							}
							delete iter;
							iter = iterNext;
						}
					}
				}

				if (!found_position) 
					addPosition(docTheory, vment->eventMention);

				if (vment->eventMention->getFirstValueForSlot(Symbol(L"Crime"))) {
					transferCrime(vment->eventMention, 
						docTheory->getSentenceTheory(sentno)->getMentionSet(),
						propSet, 
						docTheory->getSentenceTheory(sentno)->getEventMentionSet());
				}

				if (!found_sentence && _matcher != 0 &&
					vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
					vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0)
				{
					for (int p = 0; p < propSet->getNPropositions(); p++) {
						const Proposition *prop = propSet->getProposition(p);
						EventSearchNode *result = _matcher->matchAllPossibleNodes(prop, CERTAIN_SENTENCES);
						if (result != 0) {
							EventSearchNode *iter = result;
							EventSearchNode *iterNext;
							while (iter != 0 && !found_sentence) {
								iterNext = iter->getNext();
								iter->setNext(0);
								const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Sentence"));
								if (node) {
									addValueMention(docTheory, vment->eventMention, node, 
										Symbol(L"Sentence"), Symbol(L"Sentence"), 1.0);
									found_sentence = true;
								}		
								delete iter;
								iter = iterNext;
							}
							delete iter;
						}
					}
				}
				vment = vment->next;
			}
		}
	}

	// Create the value mention set.  Since it's a document-level value
	// mention set, it has NULL for its tokenSequence pointer.
	ValueMentionSet *valueMentionSet = _new ValueMentionSet(NULL, _n_value_mentions);
	for (int j = 0; j < _n_value_mentions; j++) {
		valueMentionSet->takeValueMention(j, _valueMentions[j]);
	}
	docTheory->takeDocumentValueMentionSet(valueMentionSet);
}

void EnglishDeprecatedEventValueRecognizer::transferCrime(EventMention *vment, MentionSet *mentionSet,
										 const PropositionSet *propSet,
										 EventMentionSet* vmSet)
{
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition *prop = propSet->getProposition(p);
		Symbol role1 = Symbol();
		if (prop == vment->getAnchorProp())
			role1 = Symbol(L"trigger");
		else {
			for (int a = 0; a < prop->getNArgs(); a++) {
				Argument *arg = prop->getArg(a);
				if (arg->getType() == Argument::PROPOSITION_ARG && 
					arg->getProposition() == vment->getAnchorProp())
				{
					role1 = arg->getRoleSym();
					break;
				} else if (arg->getType() == Argument::MENTION_ARG && 
					arg->getMention(mentionSet)->getNode()->getHeadPreterm() 
					== vment->getAnchorNode()->getHeadPreterm())
				{
					role1 = arg->getRoleSym();
					break;
				}
			}
		}
		if (role1.is_null())
			continue;
		for (int v = 0; v < vmSet->getNEventMentions(); v++) {
			Symbol role2 = Symbol();
			EventMention *vment2 = vmSet->getEventMention(v);
			if (vment2->getFirstValueForSlot(L"Crime") != 0)
				continue;
			if (getBaseType(vment2->getEventType()) != Symbol(L"Justice"))
				continue;
			if (vment == vment2)
				continue;
			if (prop == vment2->getAnchorProp())
				role2 = Symbol(L"trigger");
			else {
				for (int a = 0; a < prop->getNArgs(); a++) {
					Argument *arg = prop->getArg(a);
					if (arg->getType() == Argument::PROPOSITION_ARG && 
						arg->getProposition() == vment2->getAnchorProp())
					{
						role2 = arg->getRoleSym();
						break;
					} else if (arg->getType() == Argument::MENTION_ARG && 
						arg->getMention(mentionSet)->getNode()->getHeadPreterm() 
						== vment2->getAnchorNode()->getHeadPreterm())
					{
						role2 = arg->getRoleSym();
						break;
					}
				}
			}
			if (role2.is_null())
				continue;
			const ValueMention *crime = vment->getFirstValueForSlot(Symbol(L"Crime"));
			if (crime != 0) {
				vment2->addValueArgument(Symbol(L"Crime"), crime, 0);
			}
		}
	}
}
										 

void EnglishDeprecatedEventValueRecognizer::addValueMention(DocTheory *docTheory, EventMention *vment, 
										   const Mention *ment, Symbol valueType, Symbol argType,
										   float score) 
{
	addValueMention(docTheory, vment, ment->getNode(), valueType, argType, score);
}

void EnglishDeprecatedEventValueRecognizer::addValueMention(DocTheory *docTheory, EventMention *vment, 
										   const SynNode *node, Symbol valueType, Symbol argType,
										   float score) 
{
	ValueMention *val = 0;
	for (int i = 0; i < _n_value_mentions; i++) {
		if ((_valueMentions[i]->getSentenceNumber() == vment->getSentenceNumber()) &&
			(_valueMentions[i]->getFullType().getNameSymbol() == valueType || _valueMentions[i]->getFullType().getNicknameSymbol() == valueType) &&
			(_valueMentions[i]->getStartToken() == node->getStartToken()) &&
			(_valueMentions[i]->getEndToken() == node->getEndToken()))
		{
			val = _valueMentions[i];
			break;
		}
	}
	
	if (val == 0 && _n_value_mentions < MAX_DOCUMENT_VALUES) {
		try {
			ValueMentionUID uid = ValueMention::makeUID(docTheory->getNSentences(), _n_value_mentions);
			val = _new ValueMention(vment->getSentenceNumber(), uid, node->getStartToken(), node->getEndToken(), valueType);
			_valueMentions[_n_value_mentions++] = val;
		} catch (InternalInconsistencyException &e) {
			SessionLogger::err("value_mention_uid") << e;
		}
	}

	if (val != 0)
		vment->addValueArgument(argType, val, score);
	
}

const Mention *EnglishDeprecatedEventValueRecognizer::getNthMentionFromProp(SentenceTheory *sTheory, 
														   const Proposition *prop, int n) 
{
	Argument *arg = prop->getArg(n);
	if (arg->getType() == Argument::MENTION_ARG) {
		return arg->getMention(sTheory->getMentionSet());
	} else return 0;
}

const SynNode *EnglishDeprecatedEventValueRecognizer::getNthPredHeadFromProp(const Proposition *prop, int n) {
	Argument *arg = prop->getArg(n);
	if (arg->getType() == Argument::PROPOSITION_ARG) {
		const SynNode *pred = arg->getProposition()->getPredHead();
		while (true) {
			if (pred->getParent() != 0 && pred->getParent()->getHead() == pred) 
				pred = pred->getParent();
			else break;
		}
		return pred;
	} else return 0;
}

void EnglishDeprecatedEventValueRecognizer::addPosition(DocTheory* docTheory, EventMention *vment) {
	if (getBaseType(vment->getEventType()) != Symbol(L"Personnel"))
		return;

	SentenceTheory *thisSentence = docTheory->getSentenceTheory(vment->getSentenceNumber());	

	for (int i = 0; i < vment->getNArgs(); i++) {
		if (vment->getNthArgRole(i) == Symbol(L"Person")) {
			const Mention *personMent = vment->getNthArgMention(i);
			if (personMent->mentionType == Mention::DESC &&
				isJobTitle(personMent->getNode()->getHeadWord())) 
			{
				addValueMention(docTheory, vment, personMent, 
					Symbol(L"Job-Title"), Symbol(L"Position"), .5);
				return;
			}
			Entity *entity = docTheory->getEntitySet()->getEntityByMention(personMent->getUID());
			if (entity == 0)
				continue;
			for (int j = 0; j < entity->getNMentions(); j++) {
				if (Mention::getSentenceNumberFromUID(entity->getMention(j))
					== vment->getSentenceNumber()) 
				{
					int index = Mention::getIndexFromUID(entity->getMention(j));
					const Mention *ment = thisSentence->getMentionSet()->getMention(index);
					if (ment->mentionType == Mention::DESC && 
						isJobTitle(ment->getNode()->getHeadWord()))
					{
						addValueMention(docTheory, vment, ment, Symbol(L"Job-Title"), 
							Symbol(L"Position"), .5);
						return;
					}
				}
			}
		}
	}
}

bool EnglishDeprecatedEventValueRecognizer::isJobTitle(Symbol word) {
	WordNet *wordNet = WordNet::getInstance();

	if (wordNet->isHyponymOf(word, "leader"))
		return true;
	if (wordNet->isHyponymOf(word, "worker"))
		return true;

	return false;
}

Symbol EnglishDeprecatedEventValueRecognizer::getBaseType(Symbol fullType) {
	std::wstring str = fullType.to_string();
	size_t index = str.find(L".");
	return Symbol(str.substr(0, index).c_str());
}
