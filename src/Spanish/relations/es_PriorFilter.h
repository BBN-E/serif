// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PRIORFILTER_H
#define es_PRIORFILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

class PriorFilter : public RelationFilter {

public:
  PriorFilter() : RelationFilter(1) { 
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst, Symbol indicator ) {
	  _probEvent[0] = inst->getRelationType();
	  return _probEvent;
  }

  
};

#endif
