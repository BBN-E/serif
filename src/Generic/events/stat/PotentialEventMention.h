// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POTENTIAL_EVENT_MENTION_H
#define POTENTIAL_EVENT_MENTION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"

class PotentialEventMention {
public:
	PotentialEventMention() {}
	~PotentialEventMention() {}
	
	void setTriggerAndType(Symbol type, int start, int end) {
		_type = type;
		_trigger_start = start; 
		_trigger_end = end; 
	}


	Symbol getEventType() { return _type; }
	int getTriggerStart() { return _trigger_start; }
	int getTriggerEnd() { return _trigger_end; }

private:
	Symbol _type;
	int _trigger_start;
	int _trigger_end;
};


#endif
