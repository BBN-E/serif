// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_CONSTRUCTIONFILTER_H
#define EN_CONSTRUCTIONFILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

class ConstructionFilter : public RelationFilter {

public:
  ConstructionFilter() : RelationFilter(2) { 
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {

	  _probEvent[0] = inst->getPredicationType();
	  _probEvent[1] = inst->getRelationType();
	  return _probEvent;
  }

  
};

#endif
