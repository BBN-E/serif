// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_WORDNET_VECTOR_FILTER_H
#define ET_WORDNET_VECTOR_FILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ETWordNetVectorFilter : public EventTriggerFilter {

public:
  ETWordNetVectorFilter() : EventTriggerFilter(4) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 2;
	  _uniqueMultipliers[2] = 2;
  }
  
  Symbol* getSymbolVector( EventTriggerObservation *observation , Symbol eventType) {

	  _probEvent[0] = eventType;
	  int top_offset = observation->getReversedNthOffset(1);
	  int next_offset = observation->getReversedNthOffset(4);
	  if (top_offset != -1) {
		  wchar_t buffer[100];
		  swprintf(buffer, 100, L"%d", top_offset);
		  _probEvent[1] = Symbol(buffer);
	  } else _probEvent[1] = Symbol(L":NONE");	  
	  if (next_offset != -1) {
		  wchar_t buffer[100];
		  swprintf(buffer, 100, L"%d", next_offset);
		  _probEvent[2] = Symbol(buffer);
	  } else _probEvent[2] = Symbol(L":NONE");	  
	  _probEvent[3] = observation->getStemmedWord();
	  return _probEvent;
  }

  
};

#endif
