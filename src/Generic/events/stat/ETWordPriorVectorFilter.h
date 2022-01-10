// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_WORD_PRIOR_VECTOR_FILTER_H
#define ET_WORD_PRIOR_VECTOR_FILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ETWordPriorVectorFilter : public EventTriggerFilter {

public:
  ETWordPriorVectorFilter() : EventTriggerFilter(3) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 2;
  }
  
  Symbol* getSymbolVector( EventTriggerObservation *observation , Symbol eventType) {

	  _probEvent[0] = eventType;
	  _probEvent[1] = observation->getStemmedWord();
	  _probEvent[2] = observation->getLCWord();
	  return _probEvent;  
  }
  
};

#endif
