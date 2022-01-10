// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NODEFILTER_H
#define EN_NODEFILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class NodeFilter : public RelationFilter {

public:
  NodeFilter() : RelationFilter(4) { 
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {

	  wchar_t buffer[500];

	  if (indicator == Symbol(L"LEFT")) {
		  _probEvent[0] = inst->getLeftEntityType();
		  swprintf(buffer, 500, L"%ls:LEFT",
			  inst->getRelationType().to_string());
		  _probEvent[2] = inst->getLeftRole();
	  } else if (indicator == Symbol(L"RIGHT")) {
		  _probEvent[0] = inst->getRightEntityType();
		  swprintf(buffer, 500, L"%ls:RIGHT",
			  inst->getRelationType().to_string());		
		  _probEvent[2] = inst->getRightRole();
	  } else {
		  _probEvent[0] = inst->getNestedWord();
		  swprintf(buffer, 500, L"%ls:RIGHT",
			  inst->getRelationType().to_string());
		  _probEvent[2] = inst->getNestedRole();
	  } 

	  _probEvent[1] = Symbol(buffer);
	  _probEvent[3] = inst->getStemmedPredicate();
	  return _probEvent;
  }

  
};

#endif
