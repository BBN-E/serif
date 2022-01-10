// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/InternalInconsistencyException.h"
#include "common/OutputUtil.h"
#include "theories/EventSet.h"
#include "theories/Event.h"

#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

using namespace std;

EventSet::~EventSet() {
	for (int i = 0; i < _events.length(); i++) {
		delete _events[i];
		_events[i] = NULL;
	}
}

EventSet::EventSet(EventSet &other) : _eventType(other._eventType) {
	for (int i = 0; i < other._events.length(); i++)
		_events.add(_new Event(*other._events[i]));
}

EventSet::EventSet(std::vector<EventSet*> splitEventSets, std::vector<int> sentenceOffsets, std::vector<EventMentionSet*> mergedEventMentionSets) {
	int event_set_offset = 0;
	for (size_t es = 0; es < splitEventSets.size(); es++) {
		EventSet* splitEventSet = splitEventSets[es];
		if (splitEventSet != NULL) {
			for (int e = 0; e < splitEventSet->getNEvents(); e++) {
				Event* ev = _new Event(*(splitEventSet->getEvent(e)), event_set_offset, sentenceOffsets[es], mergedEventMentionSets);
				_events.add(ev);
			}
			event_set_offset += splitEventSet->getNEvents();
		}
	}
}

float EventSet::getScore() {
	float score = 0;
	for (int i = 0; i < _events.length(); i++)
		score += _events[i]->getScore();
	return score;
}


void EventSet::takeEvent(Event *event) {
	_events.add(event);
}

Event *EventSet::getEvent(int i) const {
	if ((unsigned) i < (unsigned) _events.length())
		return _events[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"EventSet::getEvent()", _events.length(), i);
}


void EventSet::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Event Set:";

	if (_events.length() == 0) {
		out << newline << "  (no events)";
	}
	else {
		int i;
		for (i = 0; i < _events.length(); i++) {
			out << newline << "- ";
			_events[i]->dump(out, indent + 2);
		}
	}

	delete[] newline;
}


void EventSet::updateObjectIDTable() const {

	ObjectIDTable::addObject(this);
	for (int i = 0; i < _events.length(); i++)
		_events[i]->updateObjectIDTable();

}

void EventSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"EventSet", this);

	stateSaver->saveInteger(_events.length());
	stateSaver->beginList(L"EventSet::_events");
	for (int i = 0; i < _events.length(); i++)
		_events[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

EventSet::EventSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"EventSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	int n_events = stateLoader->loadInteger();
	_events.setLength(n_events);
	stateLoader->beginList(L"EventSet::_events");
	for (int i = 0; i < n_events; i++)
		_events[i] = _new Event(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void EventSet::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < _events.length(); i++)
		_events[i]->resolvePointers(stateLoader);
}

const wchar_t* EventSet::XMLIdentifierPrefix() const {
	return L"eventset";
}

void EventSet::saveXML(SerifXML::XMLTheoryElement eventsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context == 0)
		throw InternalInconsistencyException("EventSet::saveXML", "Expected context to be a DocTheory");
	if (!_eventType.is_null())
		eventsetElem.setAttribute(X_event_type, _eventType);
	for (int i = 0; i < getNEvents(); i++) {
		eventsetElem.saveChildTheory(X_Event, getEvent(i), context);
		if (getEvent(i)->getID() != i)
			throw InternalInconsistencyException("EventSet::saveXML", 
				"Unexpected Event::_ID value");
	}
}

EventSet::EventSet(SerifXML::XMLTheoryElement eventSetElem)
: _events(0)
{
	using namespace SerifXML;
	eventSetElem.loadId(this);
	_eventType = eventSetElem.getAttribute<Symbol>(X_event_type, Symbol());

	XMLTheoryElementList eventElems = eventSetElem.getChildElementsByTagName(X_Event);
	int n_events = static_cast<int>(eventElems.size());
	_events.setLength(n_events);
	for (int i=0; i<n_events; ++i)
		_events[i] = _new Event(eventElems[i], i);
}
