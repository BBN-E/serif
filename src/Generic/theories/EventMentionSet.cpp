// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/InternalInconsistencyException.h"
#include "common/OutputUtil.h"
#include "theories/EventMentionSet.h"
#include "theories/EventMention.h"


#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

using namespace std;

EventMentionSet::EventMentionSet(const EventMentionSet &other, int sent_offset, int event_offset, const Parse* parse, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet, const PropositionSet* propSet, const ValueMentionSet* documentValueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap)
: _score(other._score)
{
	_parse = parse;

	BOOST_FOREACH(EventMention* otherEventMention, other._ementions) {
		_ementions.push_back(_new EventMention(*otherEventMention, sent_offset, event_offset, parse, mentionSet, valueMentionSet, propSet, documentValueMentionSet, documentValueMentionMap));
	}
}

EventMentionSet::~EventMentionSet() {
	for (size_t i = 0; i < _ementions.size(); i++)
		delete _ementions.at(i);
	_ementions.clear();
}

void EventMentionSet::takeEventMention(EventMention *emention) {
	// set the UID based on how many event mentions already exist in this set
	int sentno = emention->getUID().sentno();
	int index = static_cast<int>(_ementions.size());
	emention->setUID(sentno, index);
	_score += emention->getScore();
	_ementions.push_back(emention);	
}

void EventMentionSet::takeEventMentions(EventMentionSet *eventmentionset) {
	for (int i = 0; i < eventmentionset->getNEventMentions(); i++) {
		takeEventMention(eventmentionset->getEventMention(i));
	}
	eventmentionset->clear();
}

EventMention *EventMentionSet::getEventMention(int i) const {
	if ((unsigned) i < (unsigned) _ementions.size())
		return _ementions.at(i);
	else
		throw InternalInconsistencyException::arrayIndexException(
			"EventMentionSet::getEventMention()", _ementions.size(), i);
}

EventMention *EventMentionSet::findEventMentionByUID(EventMentionUID uid) const {
	BOOST_FOREACH(EventMention* em, _ementions) {
		if (em->getUID() == uid)
			return em;
	}
	return 0;
}


void EventMentionSet::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Event Mention Set:";

	if (_ementions.size() == 0) {
		out << newline << "  (no event mentions)";
	}
	else {
		BOOST_FOREACH(EventMention* em, _ementions) {
			out << newline << "- ";
			em->dump(out, indent + 2);
		}
	}

	delete[] newline;
}

void EventMentionSet::updateObjectIDTable() const {

	ObjectIDTable::addObject(this);
	BOOST_FOREACH(EventMention* em, _ementions) {
		em->updateObjectIDTable();
	}

}

void EventMentionSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"EventMentionSet", this);

	// EventMention loading was crashing for me on state files because we were writing
	// in the order parseId #ementions, then reading in the opposite. I'm 
	// making the writing match the reading. Note this will still crash for old
	// state files which lack the parse ID which is now required. ~ RMG
	
	stateSaver->saveInteger(_ementions.size());
	if (_ementions.size() > 0) {
		stateSaver->savePointer(_parse);
	}
	
	stateSaver->beginList(L"EventMentionSet::_ementions");
	BOOST_FOREACH(EventMention* em, _ementions) {
		em->saveState(stateSaver);
	}
	stateSaver->endList();

	stateSaver->endList();
}
EventMentionSet::EventMentionSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"EventMentionSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	

	int n_ementions = stateLoader->loadInteger();

	if (n_ementions > 0) {
		_parse = static_cast<Parse *>(stateLoader->loadPointer());
	} else {
		_parse = 0;
	}

	stateLoader->beginList(L"EventMentionSet::_ementions");
	for (int i = 0; i < n_ementions; i++) {
		EventMention *em = _new EventMention(0,0);
		em->loadState(stateLoader);
		_ementions.push_back(em);
	}
	stateLoader->endList();

	stateLoader->endList();
}

void EventMentionSet::resolvePointers(StateLoader * stateLoader) {
	_parse = static_cast<Parse *>(stateLoader->getObjectPointerTable().getPointer(_parse));
	BOOST_FOREACH(EventMention* em, _ementions) {
		em->resolvePointers(stateLoader);
	}
}

const wchar_t* EventMentionSet::XMLIdentifierPrefix() const {
	return L"eventmentionset";
}

void EventMentionSet::saveXML(SerifXML::XMLTheoryElement eventmentionsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("EventMentionSet::saveXML", "Expected context to be NULL");
	eventmentionsetElem.setAttribute(X_score, _score);
	if (_parse != 0){
		eventmentionsetElem.saveTheoryPointer(X_parse_id, _parse);
	}
	int n_ementions = static_cast<int>(_ementions.size());
	for (int i=0; i<n_ementions; ++i) {
		eventmentionsetElem.saveChildTheory(X_EventMention, _ementions.at(i), this);
		if (_ementions.at(i)->getUID().index() != i)
			throw InternalInconsistencyException("EventMentionSet::saveXML", 
				"Unexpected EventMention UID value");
	}
}

EventMentionSet::EventMentionSet(SerifXML::XMLTheoryElement emSetElem, int sent_no)
: _score(0)
{
	using namespace SerifXML;
	emSetElem.loadId(this);
	_score = emSetElem.getAttribute<float>(X_score, 0);

	_parse = emSetElem.loadOptionalTheoryPointer<Parse>(X_parse_id);
	XMLTheoryElementList emElems = emSetElem.getChildElementsByTagName(X_EventMention);
	int n_ementions = static_cast<int>(emElems.size());
	_ementions.resize(n_ementions);
	for (int i=0; i<n_ementions; ++i) {
		_ementions.at(i) = _new EventMention(emElems[i], sent_no, i);
	}
}

void EventMentionSet::resolvePointers(SerifXML::XMLTheoryElement emSetElem) {
	using namespace SerifXML;
	XMLTheoryElementList emElems = emSetElem.getChildElementsByTagName(X_EventMention);
	if (_parse != 0){
		_parse = emSetElem.loadTheoryPointer<Parse>(X_parse_id);
	}
	int n_ementions = static_cast<int>(_ementions.size());
	for (int i=0; i<n_ementions; ++i)
		_ementions.at(i)->resolvePointers(emElems[i], _parse);
}
