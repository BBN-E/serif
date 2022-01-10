// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_FEATUREVECTORFILTER_H
#define XX_FEATUREVECTORFILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class GenericFeatureVectorFilter : public RelationFilter {

public:
  GenericFeatureVectorFilter() : RelationFilter(3) { 
	  _uniqueMultipliers[0] = 2;
	  _uniqueMultipliers[1] = 0; // irrelevant (to prior)
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {

	  _probEvent[0] = inst->getRelationType();
	  wchar_t buffer[100];
	  swprintf(buffer, 100, L"%ls:%ls",
			   inst->getLeftEntityType().to_string(), 
			   inst->getRightEntityType().to_string());
	  wchar_t full_buffer[200];
	  if (inst->isNested()) {
		  swprintf(full_buffer, 200, L"%ls:%ls:%ls@%ls",
			  buffer,
			  inst->getLeftRole().to_string(),
			  inst->getNestedRole().to_string(),
			  inst->getRightRole().to_string());
	  } else {
		  swprintf(full_buffer, 200, L"%ls:%ls:%ls", buffer,
			  inst->getLeftRole().to_string(), 
			  inst->getRightRole().to_string());
	  }	  
	  _probEvent[1] = Symbol(full_buffer);
	  _probEvent[2] = inst->getStemmedPredicate();
	  return _probEvent;
  }

  
};

#endif
