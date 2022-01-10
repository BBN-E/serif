// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PREDICATEFILTER_H
#define es_PREDICATEFILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class PredicateFilter : public RelationFilter {

public:
  PredicateFilter() : RelationFilter(2) { 
	  _uniqueMultipliers[0] = 3;
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {

	  _probEvent[0] = inst->getStemmedPredicate();
	  wchar_t buffer[500];
	  swprintf(buffer, 500, L"%ls:%ls",
		  inst->getPredicationType().to_string(), 
		  inst->getRelationType().to_string());
	  _probEvent[1] = Symbol(buffer);
	  return _probEvent;
  }

  
};

#endif
