// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/EventMention.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>

namespace ICEWS {

Symbol ICEWSEventMention::NEUTRAL_TENSE = Symbol(L"neutral");
Symbol ICEWSEventMention::CURRENT_TENSE = Symbol(L"current");
Symbol ICEWSEventMention::HISTORICAL_TENSE = Symbol(L"historical");
Symbol ICEWSEventMention::ONGOING_TENSE = Symbol(L"ongoing");

ICEWSEventMention::ICEWSEventMention(ICEWSEventType_ptr eventType, const ParticipantMap& participants, Symbol patternId, Symbol tense) 
: _eventType(eventType), _participants(participants), _patternId(patternId), _event_tense(tense)
{
}

ActorMention_ptr ICEWSEventMention::getParticipant(Symbol role) const {
	ParticipantMap::const_iterator it = _participants.find(role);
	if (it == _participants.end())
		return ActorMention_ptr();
	else
		return (*it).second;
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

bool ICEWSEventMention::isValidTense(Symbol sym) {
	return (sym ==  NEUTRAL_TENSE || sym ==  CURRENT_TENSE ||
		sym ==  HISTORICAL_TENSE || sym ==  ONGOING_TENSE);
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
	Symbol eventCode = elem.getAttribute<Symbol>(X_eventCode);
	_eventType = ICEWSEventType::getEventTypeForCode(eventCode);
	_patternId = elem.getAttribute<Symbol>(X_patternId);
	_event_tense = elem.getAttribute<Symbol>(X_eventTense);
	std::vector<XMLTheoryElement> participantElems = elem.getChildElementsByTagName(X_ICEWSEventParticipant);
	BOOST_FOREACH(XMLTheoryElement pElem, participantElems) {
		Symbol role = pElem.getAttribute<Symbol>(X_role);
		if (_participants.find(role) != _participants.end()) {
			std::wostringstream err;
			err << L"Role " << role << " assigned multiple values";
			elem.reportLoadError(err.str().c_str());
		}
		// Load the actor, and use shared_from_this to get its shared pointer.
		ActorMention* participant = pElem.loadNonConstTheoryPointer<ActorMention>(X_actor_id); 
		_participants[role] = participant->shared_from_this();
	}
}

void ICEWSEventMention::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;
	static const XMLCh* X_ICEWSEventParticipant = xercesc::XMLString::transcode("ICEWSEventParticipant");
	static const XMLCh* X_actor_id = xercesc::XMLString::transcode("actor_id");
	static const XMLCh* X_eventCode = xercesc::XMLString::transcode("event_code");
	static const XMLCh* X_patternId = xercesc::XMLString::transcode("pattern_id");
	static const XMLCh* X_eventTense = xercesc::XMLString::transcode("event_tense");
	elem.setAttribute(X_eventCode, _eventType->getEventCode());
	elem.setAttribute(X_patternId, _patternId);
	elem.setAttribute(X_eventTense, _event_tense);

	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		ActorMention_ptr actor = pair.second;
		XMLTheoryElement childElem = elem.addChild(X_ICEWSEventParticipant);
		childElem.saveTheoryPointer(X_actor_id, actor.get());
		childElem.setAttribute(X_role, pair.first);
	}
}

bool ICEWSEventMention::isDatabaseWorthyEvent() const {
	typedef std::pair<Symbol, ActorMention_ptr> SymbolActorPair;
	BOOST_FOREACH(SymbolActorPair pair, _participants) {
		if (CompositeActorMention_ptr cam_actor = boost::dynamic_pointer_cast<CompositeActorMention>(pair.second)) {
			if (cam_actor->getPairedActorId().isNull())
				return false;
		}				
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
		++sent_no_count[actor->getIcewsSentNo(docTheory)];
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


} // end of ICEWS namespace


std::wostream & operator << ( std::wostream &stream, ICEWS::ICEWSEventMention_ptr eventMention ) {
	eventMention->dump(stream);
	return stream;
}

std::ostream & operator << ( std::ostream &stream, ICEWS::ICEWSEventMention_ptr eventMention ) {
	std::wostringstream s;
	eventMention->dump(s);
	stream << UnicodeUtil::toUTF8StdString(s.str());
	return stream;
}

