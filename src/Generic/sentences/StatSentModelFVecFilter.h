// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_MODEL_F_VEC_FILTER_H
#define STAT_SENT_MODEL_F_VEC_FILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/sentences/StatSentModelFilter.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


class StatSentModelFVecFilter : public StatSentModelFilter {

public:
	StatSentModelFVecFilter() : StatSentModelFilter(4) { 
		// try leaving multipliers as they are
		//_uniqueMultipliers[0] = 2;
	}
  
	Symbol* getSymbolVector(StatSentModelInstance *inst) {
		_probEvent[0] = inst->getTag();
		_probEvent[1] = inst->getWord();
		_probEvent[2] = inst->getWord1();
		_probEvent[3] = inst->getWord2();

		return _probEvent;
	}
};

#endif
