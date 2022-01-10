// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "common/leak_detection.h"

#include "Generic/CASerif/correctanswers/CorrectEvent.h"
#include "Generic/CASerif/correctanswers/CorrectEventMention.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "common/Sexp.h"
#include "common/UnexpectedInputException.h"

CorrectEvent::CorrectEvent() { 
	_system_event_id = -1;
}

CorrectEvent::~CorrectEvent() 
{
	delete [] _mentions;
}

void CorrectEvent::loadFromSexp(Sexp *eventSexp)
{
	int num_children = eventSexp->getNumChildren();
	if (num_children < 6 )
		throw UnexpectedInputException("CorrectEvent::loadFromSexp()",
									   "eventSexp doesn't have at least 6 children");

	Sexp *typeSexp = eventSexp->getNthChild(0);
	Sexp *modSexp = eventSexp->getNthChild(1);
	Sexp *genSexp = eventSexp->getNthChild(2);
	Sexp *tenSexp = eventSexp->getNthChild(3);
	Sexp *polSexp = eventSexp->getNthChild(4);
	Sexp *eventIDSexp = eventSexp->getNthChild(5);

	if (!typeSexp->isAtom() || !eventIDSexp->isAtom() ||
		!modSexp->isAtom() || !genSexp->isAtom() || !polSexp->isAtom() ||
		!tenSexp->isAtom())
		throw UnexpectedInputException("CorrectEvent::loadFromSexp()",
									   "Didn't find event atoms in correctAnswerSexp");


	_annotationEventID = eventIDSexp->getValue();
	_eventType = typeSexp->getValue();
	_modality = modSexp->getValue();
	_genericity = genSexp->getValue();
	_polarity = polSexp->getValue();
	_tense = tenSexp->getValue();
	
	_n_mentions = num_children - 6;
	int i;
	
	if (_n_mentions > 0) 
		_mentions = _new CorrectEventMention[_n_mentions];

	for (i = 0; i < _n_mentions; i++ ) {
		Sexp* mentionSexp = eventSexp->getNthChild(i + 6);
		_mentions[i].loadFromSexp(mentionSexp);
		_mentions[i].setCorrectEvent(this);
	}
}
/*
void CorrectEvent::loadFromAdept(Event *eventAdept)
{
	int num_children = eventERE->getNumChildren();
	if (num_children < 6 )
		throw UnexpectedInputException("CorrectEvent::loadFromSexp()",
									   "eventSexp doesn't have at least 6 children");

	Sexp *typeSexp = eventSexp->getNthChild(0);
	Sexp *modSexp = eventSexp->getNthChild(1);
	Sexp *genSexp = eventSexp->getNthChild(2);
	Sexp *tenSexp = eventSexp->getNthChild(3);
	Sexp *polSexp = eventSexp->getNthChild(4);
	Sexp *eventIDSexp = eventSexp->getNthChild(5);

	if (!typeSexp->isAtom() || !eventIDSexp->isAtom() ||
		!modSexp->isAtom() || !genSexp->isAtom() || !polSexp->isAtom() ||
		!tenSexp->isAtom())
		throw UnexpectedInputException("CorrectEvent::loadFromSexp()",
									   "Didn't find event atoms in correctAnswerSexp");

     long id=eventAdept.eventId;

	 std::string idString;
    std::stringstream strstream;
    strstream << id;
    strstream >> idString;

	_annotationEventID = Symbol(string_to_wstring(idString));

	_eventType =  eventAdept.eventType.type;

	_modality = modSexp->getValue();
	_genericity = genSexp->getValue();
	_polarity = polSexp->getValue();
	_tense = tenSexp->getValue();
	
	_n_mentions = 1;

	int i;
	
	if (_n_mentions > 0) 
		_mentions = _new CorrectEventMention[_n_mentions];

	for (i = 0; i < _n_mentions; i++ ) {
		_mentions[i].loadFromAdept(eventAdept.arguments);
		_mentions[i].setCorrectEvent(this);
	}
}*/
