// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_OBJECT_ARG_VECTOR_FILTER_H
#define ET_OBJECT_ARG_VECTOR_FILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ETObjectArgVectorFilter : public EventTriggerFilter {

public:
  ETObjectArgVectorFilter() : EventTriggerFilter(3) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 2;
  }
  
  Symbol* getSymbolVector( EventTriggerObservation *observation , Symbol eventType) {

	  _probEvent[0] = eventType;

	  wchar_t buffer[100];
	  if (!observation->getObjectOfTrigger().is_null()) {
		  swprintf(buffer, 100, L"OBJ:%ls", 
			  observation->getObjectOfTrigger().to_string());
		  _probEvent[1] = Symbol(buffer);
	  } else {
		  _probEvent[1] = Symbol(L":NONE");
	  }

	  _probEvent[2] = observation->getStemmedWord();
	  return _probEvent;  
  }
  
};

#endif
