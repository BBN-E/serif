// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_CLUSTER_16_VECTOR_FILTER_H
#define ET_CLUSTER_16_VECTOR_FILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ETCluster16VectorFilter : public EventTriggerFilter {

public:
  ETCluster16VectorFilter() : EventTriggerFilter(3) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 2;
  }
  
  Symbol* getSymbolVector( EventTriggerObservation *observation , Symbol eventType) {

	  _probEvent[0] = eventType;
	  int wc16 = observation->getWordCluster().c16();
	  if (wc16 != 0) {
		  wchar_t buffer[100];
		  swprintf(buffer, 99, L"%d", wc16);
		  _probEvent[1] = Symbol(buffer);
	  } else _probEvent[1] = Symbol(L":NONE");	  	  
	  _probEvent[2] = observation->getStemmedWord(); 
	  return _probEvent;
  }

  
};

#endif
