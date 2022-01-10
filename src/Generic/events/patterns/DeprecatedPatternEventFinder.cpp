// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/events/patterns/EventPatternSet.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/events/patterns/EventPatternMatcher.h"
#include "Generic/events/patterns/EventSearchNode.h"
#include "Generic/events/EventLinker.h"
#include "Generic/events/EventFinder.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/events/patterns/SurfaceLevelSentence.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

UTF8OutputStream DeprecatedPatternEventFinder::_debugStream;
bool DeprecatedPatternEventFinder::DEBUG = false;
int DeprecatedPatternEventFinder::_sentence_num;
Symbol DeprecatedPatternEventFinder::_docName;

CorrectAnswers* DeprecatedPatternEventFinder::_correctAnswers;

Symbol DeprecatedPatternEventFinder::BLOCK_SYM(L"BLOCK");
Symbol DeprecatedPatternEventFinder::FORCED_SYM(L"FORCED");

DeprecatedPatternEventFinder::DeprecatedPatternEventFinder() : _matcher(0)
{
	
	std::string rules = ParamReader::getParam("event_patterns");
	EventPatternSet *set = _new EventPatternSet(rules.c_str());
	_matcher = _new EventPatternMatcher(set);

	std::string buffer = ParamReader::getParam("event_pattern_debug");
	DEBUG = (!buffer.empty());
	if (DEBUG) _debugStream.open(buffer.c_str());

	TOPLEVEL_PATTERNS = Symbol(L"TOPLEVEL_PATTERNS");

	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

}

DeprecatedPatternEventFinder::~DeprecatedPatternEventFinder() {
	delete _matcher;
}

void DeprecatedPatternEventFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num)
{
	_sentence_num = sentence_num;
	_docName = docTheory->getDocument()->getName();
	if (sentence_num == 0) {
		//_prevSet = 0;
	}
	else {
        SentenceTheory *lastTheory = docTheory->getSentenceTheory(sentence_num-1);
		//_prevSet = lastTheory->getEventSet();

	}

}

EventMentionSet *DeprecatedPatternEventFinder::getEventTheory(const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       const PropositionSet *propSet)

{
	if (!_matcher) return _new EventMentionSet(parse);

	// This really does need to be done every time, because it is dealing with
	//   a static function that someone else may have reset in the meanwhile!
	if (_allow_mention_set_changes)
		EventSearchNode::allowMentionSetChanges();
	else EventSearchNode::disallowMentionSetChanges();

	for (int e = 0; e < MAX_SENTENCE_TOKENS; e++) {
		_event_found[e] = false;
	}

	_matcher->resetForNewSentence(propSet, mentionSet, valueMentionSet);
	_surfaceSentence = _new SurfaceLevelSentence(parse->getRoot(), mentionSet);
	std::vector<bool> is_false_or_hypothetical = RelationUtilities::get()->identifyFalseOrHypotheticalProps(propSet, mentionSet);

	if (DEBUG) {
		_debugStream << L"*** NEW SENTENCE ***\n";
		_debugStream << _surfaceSentence->toString() << L"\n";
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			propSet->getProposition(i)->dump(_debugStream, 0);
			if (is_false_or_hypothetical[propSet->getProposition(i)->getIndex()])
				_debugStream << L" -- UNTRUTH";
			_debugStream << L"\n";
		}
		_debugStream << parse->getRoot()->toTextString() << L"\n";
		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *ment = mentionSet->getMention(j);
			if (ment->getEntityType().isRecognized()) {
				_debugStream << ment->getNode()->toTextString() << ": "
							 << ment->getEntityType().getName().to_string()
							 << "\n";
			}
		}
		_debugStream << parse->getRoot()->toPrettyParse(0) << L"\n";

	}

	_n_events = 0;
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		// should this somehow go deeper than this?
		if (!_use_correct_answers) {
			if (is_false_or_hypothetical[prop->getIndex()])
				continue;
		}

		EventSearchNode *result = _matcher->matchAllPossibleNodes(prop, TOPLEVEL_PATTERNS);
		if (result != 0) {
			EventSearchNode *iter = result;
			EventSearchNode *iterNext;
			while (iter != 0) {
				iterNext = iter->getNext();
				iter->setNext(0);
				EventMention *mention
					= _new EventMention(mentionSet->getSentenceNumber(), _n_events);
				iter->populateEventMention(mention, mentionSet, valueMentionSet);
				mention->setScore(-0.8F);
				if (mention->getEventType() != SymbolConstants::nullSymbol && _n_events < MAX_SENTENCE_EVENTS) {
					_events[_n_events] = mention;
					_n_events++;
					if (DEBUG) {
						_debugStream << L"FOUND:\n" << iter->toString();
						_debugStream << mention->toString() << L"\n";
					}
				} else {
					delete mention;
					if (DEBUG) {
						_debugStream << L"no event type (probably blocked):\n";
						_debugStream << iter->toString() << L"\n";
					}
				}
				delete iter;
				iter = iterNext;
			}
		}
	}
/*
if (_use_correct_answers) {
	cullIncorrectEvents(mentionSet, entitySet);
	findMissingCorrectEvents(propSet, mentionSet, entitySet);
}
*/
	cullDuplicateEvents();

	int n_actual_events = 0;
	for (int k = 0; k < _n_events; k++) {
		if (_events[k] != 0) {
			EventUtilities::postProcessEventMention(_events[k], mentionSet);
			n_actual_events++;
		}
	}

	EventMentionSet *result = _new EventMentionSet(parse);
	for (int j = 0; j < _n_events; j++) {
		if (_events[j] != 0)
			result->takeEventMention(_events[j]);
		_events[j] = 0;
	}
	delete _surfaceSentence;
	return result;
}
/*
void DeprecatedPatternEventFinder::cullIncorrectEvents(const MentionSet *mentionSet, EntitySet *entitySet) {
	for (int i = 0; i < _n_events; i++) {
		if (_events[i] == 0)
			continue;
		int toknum = _events[i]->getAnchorNode()->getHeadPreterm()->getStartToken();
		Symbol systemEventType = _events[i]->getEventType();
		Symbol correctEventType = _correctAnswers->getEventType(_sentence_num, toknum, _docName);
		if (correctEventType.is_null()) {
			// try to find close-by event anchor that isn't already taken
			if (DEBUG) _debugStream << L"ANNOTATION-- wrong: " << _events[i]->toString() << L"\n";
			// if not...
			_events[i] = 0;
		} else if (systemEventType == DeprecatedPatternEventFinder::FORCED_SYM) {
			if (DEBUG) _debugStream << L"ANNOTATION-- forced (super verb): ";
			if (DEBUG) _debugStream << _events[i]->toString() << L"\n";
			EventUtilities::populateForcedEvent(_events[i], _events[i]->getAnchorProp(),
				correctEventType, mentionSet);
		} else if (systemEventType != correctEventType) {
			if (DEBUG) _debugStream << L"ANNOTATION-- wrong type: " << _events[i]->toString() << L"\n";
			EventUtilities::fixEventType(_events[i], correctEventType);
			EventUtilities::addNearbyEntities(_events[i], _surfaceSentence, entitySet);
			_event_found[toknum] = true;
		} else {
			if (DEBUG) _debugStream << L"ANNOTATION-- right: " << _events[i]->toString() << L"\n";
			EventUtilities::addNearbyEntities(_events[i], _surfaceSentence, entitySet);
			_event_found[toknum] = true;
		}
	}
}

void DeprecatedPatternEventFinder::findMissingCorrectEvents(const PropositionSet *propSet,
										   const MentionSet *mentionSet,
										   EntitySet *entitySet)
{
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);
		if (prop->getPredHead() == 0)
			continue;
		int toknum = prop->getPredHead()->getHeadPreterm()->getStartToken();
		Symbol correctEventType = _correctAnswers->getEventType(_sentence_num, toknum, _docName);
		if (!correctEventType.is_null() &&	!_event_found[toknum])
		{
			EventMention *mention = _new EventMention(_sentence_num, _n_events);
			if (DEBUG) _debugStream << L"ANNOTATION-- new: ";
			EventUtilities::populateForcedEvent(mention, prop,
				correctEventType, mentionSet);
			EventUtilities::addNearbyEntities(mention, _surfaceSentence, entitySet);
			_events[_n_events] = mention;
			_n_events++;
			if (DEBUG) _debugStream << mention->toString() << L"\n";
			_event_found[toknum] = true;
		}
	}

	for (int j = 0; j < _surfaceSentence->getLength(); j++) {
		if (_event_found[_surfaceSentence->getTokenIndex(j)])
			continue;
		Symbol correctEventType =
			_correctAnswers->getEventType(_sentence_num,
										  _surfaceSentence->getTokenIndex(j),
										  _docName);
		if (!correctEventType.is_null()) {
			EventMention *mention = _new EventMention(_sentence_num, _n_events);
			if (DEBUG) _debugStream << L"ANNOTATION-- totally faked: \n";
			mention->setEventType(correctEventType);
			// I am the most evil person ever
			Proposition *fakeProp = _new Proposition(-1, Proposition::NOUN_PRED, 1);
			fakeProp->setPredHead(_surfaceSentence->getNode(j));
			mention->setAnchor(fakeProp);
			EventUtilities::addNearbyEntities(mention, _surfaceSentence, entitySet);
			_events[_n_events] = mention;
			_n_events++;
			if (DEBUG) _debugStream << mention->toString() << L"\n";
			_event_found[_surfaceSentence->getTokenIndex(j)] = true;
		}


	}
}
*/
void DeprecatedPatternEventFinder::cullDuplicateEvents() {

	for (int i = 0; i < _n_events; i++) {
		if (_events[i] == 0)
			continue;
		if (EventUtilities::isInvalidEvent(_events[i])) {
			if (DEBUG) _debugStream << L"tossing as invalid: " << _events[i]->toString() << L"\n";
			delete _events[i];
			_events[i] = 0;
			continue;
		}
		for (int j = i + 1; j < _n_events; j++) {
			if (_events[j] == 0)
				continue;
			int compare = EventUtilities::compareEventMentions(_events[i], _events[j]);
			if (compare == 1) {
				if (DEBUG) _debugStream << L"tossing: " << _events[i]->toString() << L"\n";
				delete _events[i];
				_events[i] = 0;
				break;
			} else if (compare == 2) {
				if (DEBUG) _debugStream << L"tossing: " << _events[j]->toString() << L"\n";
				delete _events[j];
				_events[j] = 0;
				continue;
			}
		}
	}
	for (int k = 0; k < _n_events; k++) {
		if (_events[k] != 0 && _events[k]->getEventType() == BLOCK_SYM) {
			delete _events[k];
			_events[k] = 0;
		}
	}
}
