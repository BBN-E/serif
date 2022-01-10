// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_EXP_FEATUREVECTORFILTER_H
#define AR_EXP_FEATUREVECTORFILTER_H
#include "Generic/common/Symbol.h"
#include "Generic/relations/RelationFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class ArabicExpFeatureVectorFilter : public RelationFilter {

public:
  ArabicExpFeatureVectorFilter() : RelationFilter(3) { 
	  _uniqueMultipliers[0] = 2;
  	  _uniqueMultipliers[1] = 0;
  }
  
  Symbol* getSymbolVector( PotentialRelationInstance *inst , Symbol indicator) {
	  _probEvent[0] = inst->getRelationType();
	  wchar_t buffer[100];
	  swprintf(buffer, 100, L"%ls:%ls",
			   inst->getLeftEntityType().to_string(), 
			   inst->getRightEntityType().to_string());
	  _probEvent[1] = Symbol(buffer);

	  /*
	  //maybe should only have one of the headwords?
	  _probEvent[2] = inst->getLeftHeadword();
	*/	
	wchar_t hw_buffer[100];
	  swprintf(hw_buffer, 100, L"%ls:%ls",
				inst->getLeftHeadword().to_string(), 
			   inst->getRightHeadword().to_string());
	
	  _probEvent[2] = Symbol(hw_buffer);

	  return _probEvent;
  }

  
};

#endif
