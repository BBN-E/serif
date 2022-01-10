// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACRO_PERIOD_EXPANDER_H
#define ACRO_PERIOD_EXPANDER_H


#include "PropTreeExpander.h"


class AcronymPeriodDeleter : public PropTreeExpander {
public:
	AcronymPeriodDeleter() : PropTreeExpander() {}
	void expand (const PropNodes& pnodes) const;
private:
	inline static bool isAllCapsAndPeriods(const std::wstring& word);
};



#endif

