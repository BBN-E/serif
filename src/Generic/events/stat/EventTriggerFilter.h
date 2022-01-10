// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_FILTER_H
#define EVENT_TRIGGER_FILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include <iostream>


class EventTriggerFilter {
public:
	EventTriggerFilter()
		: _vectorLength(0), _probEvent(0), _uniqueMultipliers(0)
	{}
	virtual ~EventTriggerFilter() {
		delete[] _probEvent;
		delete[] _uniqueMultipliers;
	}

	float* getUniqueMultiplierArray() { return _uniqueMultipliers; }
	virtual Symbol* getSymbolVector( EventTriggerObservation *observation, Symbol eventType) = 0; 
	int getVectorLength() { return _vectorLength; }
	void printSymbolVector( EventTriggerObservation *observation, Symbol eventType, std::ostream& out ) {
		Symbol *symVec = getSymbolVector(observation, eventType);
		out << "[ ";
		for (int i = 0; i < getVectorLength(); i++) {
			out << symVec[i].to_string() << " ";
		}
		out << "]\n";
	}

protected:
	EventTriggerFilter(int vectorLength) : _vectorLength( vectorLength ) { 
		_probEvent = _new Symbol[vectorLength];
		_uniqueMultipliers = _new float[vectorLength-1];
		for (int i = 0; i < vectorLength-1; i++)
			_uniqueMultipliers[i] = 1;
	}

	Symbol* _probEvent;
	float * _uniqueMultipliers;

private:
	int _vectorLength; 
};

#endif

