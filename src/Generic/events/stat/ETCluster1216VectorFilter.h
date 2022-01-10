// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_CLUSTER_12_16_VECTOR_FILTER_H
#define ET_CLUSTER_12_16_VECTOR_FILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ETCluster1216VectorFilter : public EventTriggerFilter {

public:
  ETCluster1216VectorFilter() : EventTriggerFilter(4) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 2;
	  _uniqueMultipliers[2] = 2;
  }
  
  Symbol* getSymbolVector( EventTriggerObservation *observation , Symbol eventType) {

	  _probEvent[0] = eventType;
	  int wc12 = observation->getWordCluster().c12();
	  int wc16 = observation->getWordCluster().c16();	  
	  if (wc12 != 0) {
		  wchar_t buffer[100];
		  swprintf(buffer, 99, L"%d", wc12);
		  _probEvent[1] = Symbol(buffer);
	  } else _probEvent[1] = Symbol(L":NONE");	  	  
	  if (wc16 != 0) {
		  wchar_t buffer[100];
		  swprintf(buffer, 99, L"%d", wc16);
		  _probEvent[2] = Symbol(buffer);
	  } else _probEvent[2] = Symbol(L":NONE");
	  _probEvent[3] = observation->getStemmedWord(); 
	  return _probEvent;
  }

  
};

#endif
