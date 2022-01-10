// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_ATTACHMENTFILTER_H
#define es_ATTACHMENTFILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class AttachmentFilter : public RelationFilter {

public:
  AttachmentFilter() : RelationFilter(3) { 
	
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {

	  wchar_t buffer[500];
	  
	  if (indicator == Symbol(L"LEFT")) {
		  _probEvent[0] = inst->getLeftRole();
		  swprintf(buffer, 500, L"%ls:LEFT",
				   inst->getRelationType().to_string());		
		  _probEvent[2] = inst->getStemmedPredicate();
	  } else if (indicator == Symbol(L"RIGHT")) {
		  _probEvent[0] = inst->getRightRole();
		  swprintf(buffer, 500, L"%ls:RIGHT",
				   inst->getRelationType().to_string());
		  _probEvent[2] = inst->getStemmedPredicate();
	  } else {
		  _probEvent[0] = inst->getNestedRole();
		  swprintf(buffer, 500, L"%ls:RIGHT",
				   inst->getRelationType().to_string());
		  _probEvent[2] = inst->getNestedWord();
	  }

	  _probEvent[1] = Symbol(buffer);
	  return _probEvent;
  }

  
};

#endif
