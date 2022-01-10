// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_ENTITY_RELATION_H
#define EVENT_ENTITY_RELATION_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"


class EventEntityRelation : public Theory {
public:
	struct LinkedEERelMention {
		EventMention *eventMention;
		const Mention *mention;
		LinkedEERelMention *next;
		LinkedEERelMention(EventMention *em, const Mention *m) :
			eventMention(em), mention(m), next(0) {}
	};

    	EventEntityRelation(Symbol type, EventMention *eventMention, int event_id,
        	const Mention *mention, int entity_id) : _type(type), _mentions(0),
            	_event_id(event_id), _entity_id(entity_id)
    	{
        	_mentions = _new LinkedEERelMention(eventMention, mention);
        	//_uid = event_id * MAX_RELATIONS_PER_EVENT + id_counter;
        	_uid = EventEntityRelation::get_next_uid();
    	}

    	~EventEntityRelation() { delete _mentions; }

    	void addMention(EventMention *eventMention, const Mention *mention) {
        	LinkedEERelMention *last = _mentions;
        	while (last->next != 0)
            		last = last->next;
        	last->next = _new LinkedEERelMention(eventMention, mention);
    	}

	Symbol getType() { return _type; }
	LinkedEERelMention *getEEMentions() { return _mentions; }
	int getEntityID() { return _entity_id; }
	int getEventID() { return _event_id; }
	int getUID() { return _uid; }

	void updateObjectIDTable() const {
		throw InternalInconsistencyException("EventEntityRelation::updateObjectIDTable()",
											"Using unimplemented method.");
	}

	void saveState(StateSaver *stateSaver) const {
		throw InternalInconsistencyException("EventEntityRelation::saveState()",
											"Using unimplemented method.");
	}

	void resolvePointers(StateLoader * stateLoader) {
		throw InternalInconsistencyException("EventEntityRelation::resolvePointers()",
											"Using unimplemented method.");
	}
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const {
		std::cerr << "Warning: saveXML method not filled in yet!" << std::endl;
		//throw InternalInconsistencyException("EventEntityRelation::saveXML()",
		//									"Using unimplemented method.");
	}
	const wchar_t* XMLIdentifierPrefix() const {
		return L"XX";
		//throw InternalInconsistencyException("EventEntityRelation::XMLIdentifierPrefix()",
		//									"Using unimplemented method.");
	}

private:
	Symbol _type;
	LinkedEERelMention *_mentions;
	int _entity_id;
	int _event_id;
	int _uid;

    	static int uid_counter; //old unique id used #define that was axed.
    	static int get_next_uid(){ return uid_counter++; }

};

#endif
