// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_SET_H
#define EVENT_SET_H

#include "theories/SentenceTheory.h"
#include "theories/SentenceSubtheory.h"
#include "common/Symbol.h"
#include "common/GrowableArray.h"

class Event;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#include <iostream>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EventSet : public SentenceSubtheory {
public:
	EventSet() : _eventType(Symbol()) {}
	EventSet(EventSet &other);
	EventSet(std::vector<EventSet*> splitEventSets, std::vector<int> sentenceOffsets, std::vector<EventMentionSet*> mergedEventMentionSets);

	~EventSet();

	/** Add new event to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * EventSet's responsibility to delete the Event */
	void takeEvent(Event *event);

	int getNEvents() const { return _events.length(); }
	Event *getEvent(int i) const;

	Symbol getEventType() { return _eventType; }
	float getScore();

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{	return SentenceTheory::EVENT_SUBTHEORY; }


	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const EventSet &it)
		{ it.dump(out, 0); return out; }

	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	EventSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit EventSet(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;


private:
	GrowableArray <Event *> _events;
	Symbol _eventType;

};

#endif
