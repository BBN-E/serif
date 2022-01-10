// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/icews/ICEWSUtilities.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Proposition.h"
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

Symbol ICEWSEventMention::NEUTRAL_TENSE = Symbol(L"neutral");
Symbol ICEWSEventMention::CURRENT_TENSE = Symbol(L"current");
Symbol ICEWSEventMention::HISTORICAL_TENSE = Symbol(L"historical");
Symbol ICEWSEventMention::ONGOING_TENSE = Symbol(L"ongoing");
Symbol ICEWSEventMention::NULL_TENSE = Symbol(L"unavailable");
Symbol ICEWSEventMention::SOURCE_SYM = Symbol(L"SOURCE");
Symbol ICEWSEventMention::TARGET_SYM = Symbol(L"TARGET");

ICEWSEventMention::ICEWSEventMention(ICEWSEventType_ptr eventType, const ParticipantList& participants, Symbol patternId, Symbol tense, ValueMention *timeValueMention, std::vector<const Proposition *> propositions, Symbol originalEventId, bool is_reciprocal) 
  : _eventType(eventType), _participants(participants), _patternId(patternId), _event_tense(tense), _originalEventId(originalEventId), _timeValueMention(timeValueMention), _propositions(propositions), _is_reciprocal(is_reciprocal)
{}

// NOTE: This will return the first one it finds! 
ActorMention_ptr ICEWSEventMention::getParticipant(Symbol role) const {
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	for (ParticipantList::const_iterator iter = _participants.begin(); iter != _participants.end(); iter++) {
		if (iter->first == role)
			return iter->second;
	}
	return ActorMention_ptr();
}

bool ICEWSEventMention::hasBetterParticipantsThan(ICEWSEventMention_ptr other) {
	if (getParticipant(SOURCE_SYM) && getParticipant(TARGET_SYM)) {
		if (!other->getParticipant(SOURCE_SYM) || !other->getParticipant(TARGET_SYM)) {
			return true;
		} else return false;
	}

	if (getParticipant(SOURCE_SYM))
		return !other->getParticipant(SOURCE_SYM);
	
	if (getParticipant(TARGET_SYM))
		return !other->getParticipant(TARGET_SYM);

	return false;
}

bool ICEWSEventMention::hasSameEntityPlayingMultipleRoles(const DocTheory *docTheory) const {
	std::set<int> usedEntities;
	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		ActorMention_ptr actor = pair.second;
		if (!actor->getEntityMention())
			continue;
		const Entity *entity = docTheory->getEntitySet()->getEntityByMention(actor->getEntityMention()->getUID());
		if (!entity)
			continue;
		if (usedEntities.find(entity->getID()) != usedEntities.end())
			return true;
		usedEntities.insert(entity->getID());
	}
	return false;
}

ICEWSEventType_ptr ICEWSEventMention::getEventType() const {
	return _eventType;
}

void ICEWSEventMention::setEventType(ICEWSEventType_ptr newEventType) {
	if (!newEventType)
		throw InternalInconsistencyException("ICEWSEventMention::setEventType",
			"Attempt to set event type to NULL");
	_eventType = newEventType;
}

void ICEWSEventMention::setEventTense(Symbol tense) {
	_event_tense = tense;
}

Symbol ICEWSEventMention::getEventTense() const {
	return _event_tense;
}

ValueMention * ICEWSEventMention::getTimeValueMention() const {
	return _timeValueMention;
}

std::vector<const Proposition*> ICEWSEventMention::getPropositions() const {
	return _propositions;
}

bool ICEWSEventMention::isValidTense(Symbol sym) {
	return (sym ==  NEUTRAL_TENSE || sym ==  CURRENT_TENSE ||
		sym ==  HISTORICAL_TENSE || sym ==  ONGOING_TENSE || sym == NULL_TENSE);
}

Symbol ICEWSEventMention::getPatternId() const {
	return _patternId;
}

void ICEWSEventMention::updateObjectIDTable() const {
	throw InternalInconsistencyException("ICEWSEventMention::updateObjectIDTable",
		"ICEWSEventMention does not currently have state file support");
}
void ICEWSEventMention::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ICEWSEventMention::saveState",
		"ICEWSEventMention does not currently have state file support");
}
void ICEWSEventMention::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ICEWSEventMention::resolvePointers",
		"ICEWSEventMention does not currently have state file support");
}

ICEWSEventMention::ICEWSEventMention(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;
	static const XMLCh* X_ICEWSEventParticipant = xercesc::XMLString::transcode("ICEWSEventParticipant");
	static const XMLCh* X_actor_id = xercesc::XMLString::transcode("actor_id");
	static const XMLCh* X_eventCode = xercesc::XMLString::transcode("event_code");
	static const XMLCh* X_patternId = xercesc::XMLString::transcode("pattern_id");
	static const XMLCh* X_eventTense = xercesc::XMLString::transcode("event_tense");
	static const XMLCh* X_originalEventId = xercesc::XMLString::transcode("original_event_id");
	static const XMLCh* X_is_reciprocal = xercesc::XMLString::transcode("is_reciprocal");
	static const XMLCh* X_timeValueMention = xercesc::XMLString::transcode("time_value_mention_id");
	static const XMLCh* X_proposition_ids = xercesc::XMLString::transcode("proposition_ids");
	Symbol eventCode = elem.getAttribute<Symbol>(X_eventCode);
	_eventType = ICEWSEventType::getEventTypeForCode(eventCode);
	_patternId = elem.getAttribute<Symbol>(X_patternId);
	_event_tense = elem.getAttribute<Symbol>(X_eventTense);	
	_originalEventId = elem.getAttribute<Symbol>(X_originalEventId);
	_is_reciprocal = elem.getAttribute<bool>(X_is_reciprocal);
	std::vector<XMLTheoryElement> participantElems = elem.getChildElementsByTagName(X_ICEWSEventParticipant);
	BOOST_FOREACH(XMLTheoryElement pElem, participantElems) {
		Symbol role = pElem.getAttribute<Symbol>(X_role);
		// Load the actor, and use shared_from_this to get its shared pointer.
		ActorMention* participant = pElem.loadNonConstTheoryPointer<ActorMention>(X_actor_id); 
		_participants.push_back(std::make_pair(role, participant->shared_from_this()));
	}
	if(elem.hasAttribute(X_timeValueMention)){
		_timeValueMention = elem.loadNonConstTheoryPointer<ValueMention>(X_timeValueMention);
	} else{
		_timeValueMention = NULL;
    }
	if (elem.hasAttribute(X_proposition_ids)) 
		_propositions = elem.loadTheoryPointerList<Proposition>(X_proposition_ids);
}

void ICEWSEventMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;
	static const XMLCh* X_ICEWSEventParticipant = xercesc::XMLString::transcode("ICEWSEventParticipant");
	static const XMLCh* X_actor_id = xercesc::XMLString::transcode("actor_id");
	static const XMLCh* X_eventCode = xercesc::XMLString::transcode("event_code");
	static const XMLCh* X_patternId = xercesc::XMLString::transcode("pattern_id");
	static const XMLCh* X_eventTense = xercesc::XMLString::transcode("event_tense");
	static const XMLCh* X_originalEventId = xercesc::XMLString::transcode("original_event_id");
	static const XMLCh* X_is_reciprocal = xercesc::XMLString::transcode("is_reciprocal");
	static const XMLCh* X_timeValueMention = xercesc::XMLString::transcode("time_value_mention_id");
	static const XMLCh* X_proposition_ids = xercesc::XMLString::transcode("proposition_ids");
	elem.setAttribute(X_eventCode, _eventType->getEventCode());
	elem.setAttribute(X_patternId, _patternId);
	elem.setAttribute(X_eventTense, _event_tense);
	elem.setAttribute(X_originalEventId, _originalEventId);
	elem.setAttribute(X_is_reciprocal, _is_reciprocal);
	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		ActorMention_ptr actor = pair.second;
		XMLTheoryElement childElem = elem.addChild(X_ICEWSEventParticipant);
		childElem.saveTheoryPointer(X_actor_id, actor.get());
		childElem.setAttribute(X_role, pair.first);
	}
	if (_timeValueMention != 0)
		elem.saveTheoryPointer(X_timeValueMention, _timeValueMention);

	std::vector<const Theory*> propositionList;
	BOOST_FOREACH(const Proposition* prop, _propositions) {
		propositionList.push_back(prop);
	}
	elem.saveTheoryPointerList(X_proposition_ids, propositionList);
}

bool ICEWSEventMention::isDatabaseWorthyEvent() const {
	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		if (CompositeActorMention_ptr cam_actor = boost::dynamic_pointer_cast<CompositeActorMention>(pair.second)) {
			if (cam_actor->getPairedActorId().isNull())
				return false;
		} else if (ProperNounActorMention_ptr pm_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(pair.second)) {
			if (pm_actor->getActorId().isNull()) {
				return false;
			}
		} else return false;
	}
	return true;
}

size_t ICEWSEventMention::getIcewsSentNo(const DocTheory* docTheory) const {
	// Check which sentence each participant was in.  Hopefully, they'll
	// all be in the same ICEWS sentence, but there's no guarantee that 
	// this will be true since we use our own sentence segmentation.
	std::map<size_t, size_t> sent_no_count; // maps sent_no -> num_participants
	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		ActorMention_ptr actor = pair.second;
		ActorMention_ptr icewsActor = boost::dynamic_pointer_cast<ActorMention>(actor);
		++sent_no_count[ICEWSUtilities::getIcewsSentNo(icewsActor, docTheory)];
	}
	// Return the sentence number for the ICEWS sentence that has the 
	// largest number of event participants.  (In case of tie, take the
	// lowest sentence number.)
	typedef std::pair<size_t, size_t> SizeTPair;
	SizeTPair best(0,0);
	BOOST_FOREACH(SizeTPair pair, sent_no_count) {
		if (pair.second > best.second)
			best = pair;
	}
	return best.first;
}

const SentenceTheory * ICEWSEventMention::getSentenceTheory() {
	// Just take the first one
	if (_participants.empty()) 
		return 0;
	return _participants.begin()->second->getSentence();
}

void ICEWSEventMention::dump(std::wostream &stream, const char* indent) {
	stream << indent << "EVENT-" << getEventType()->getEventCode() 
		<< " (" << getEventType()->getName() << ")";
	stream << "\n" << indent << "Tense: " << _event_tense;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(ParticipantPair participantPair, _participants) {
		stream << "\n" << indent << "  " << participantPair.first << ": " 
			<< participantPair.second << "\n" << indent << "          \"" 
			<< participantPair.second->getEntityMention()->toCasedTextString() << "\"";
	}
}

std::wstring ICEWSEventMention::getEventText() const {
	// for now, just return the entire sentence.
	if (_participants.empty()) return L"???";
	const SentenceTheory* sentTheory = _participants.begin()->second->getSentence();
	return sentTheory->getTokenSequence()->toString();
}

std::wostream & operator << ( std::wostream &stream, ICEWSEventMention_ptr eventMention ) {
	eventMention->dump(stream);
	return stream;
}

std::ostream & operator << ( std::ostream &stream, ICEWSEventMention_ptr eventMention ) {
	std::wostringstream s;
	eventMention->dump(s);
	stream << UnicodeUtil::toUTF8StdString(s.str());
	return stream;
}

