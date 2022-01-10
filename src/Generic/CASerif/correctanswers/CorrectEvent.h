// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_EVENT_H
#define CORRECT_EVENT_H

#include "Generic/CASerif/correctanswers/CorrectEventMention.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/Event.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Attribute.h"


class CorrectEvent
{
private:
	
	int _system_event_id;
	Symbol _eventType;
	Symbol _modality;
	Symbol _genericity;
	Symbol _polarity;
	Symbol _tense;

	int _n_mentions;
	CorrectEventMention *_mentions;

	Symbol _annotationEventID;
	
public:
	CorrectEvent();
	~CorrectEvent();

	void loadFromSexp(Sexp *eventSexp);
	//void loadFromERE(ERE *eventERE);
	int getNMentions() { return _n_mentions; }
	CorrectEventMention * getMention(size_t index) { return &(_mentions[index]); }
	Symbol getEventType() { return _eventType; }
	int getSystemEventID() { return _system_event_id; }
	void setSystemEventID(int id) { _system_event_id = id; }
	Symbol getAnnotationEventID() { return _annotationEventID; }
	ModalityAttribute getModality() {
		if (_modality == CASymbolicConstants::VDR_MOD_ASSERTED)
			return Modality::ASSERTED;
		else if (_modality == CASymbolicConstants::VDR_MOD_OTHER)
			return Modality::OTHER;
		else return Modality::ASSERTED;
	}
	TenseAttribute getTense() {
		if (_tense == CASymbolicConstants::VDR_TENSE_FUTURE)
			return Tense::FUTURE;
		else if (_tense == CASymbolicConstants::VDR_TENSE_PAST)
			return Tense::PAST;
		else if (_tense == CASymbolicConstants::VDR_TENSE_PRESENT)
			return Tense::PRESENT;
		else if (_tense == CASymbolicConstants::VDR_TENSE_UNSPECIFIED)
			return Tense::UNSPECIFIED;
		else return Tense::UNSPECIFIED;
	}
	PolarityAttribute getPolarity() {
		if (_polarity == CASymbolicConstants::VDR_POL_NEGATIVE)
			return Polarity::NEGATIVE;
		else if (_polarity == CASymbolicConstants::VDR_POL_POSITIVE)
			return Polarity::POSITIVE;
		else return Polarity::POSITIVE;
	}	
	GenericityAttribute getGenericity() {
		if (_genericity == CASymbolicConstants::VDR_GEN_GENERIC)
			return Genericity::GENERIC;
		else if (_genericity == CASymbolicConstants::VDR_GEN_SPECIFIC)
			return Genericity::SPECIFIC;
		else return Genericity::SPECIFIC;
	}

};

#endif
