// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MORPHCHARTENTRY_H
#define AR_MORPHCHARTENTRY_H
#include "Generic/common/Symbol.h"
class MorphChartEntry{
private:

	Symbol w_2;
	Symbol w_1;
	Symbol w_0;
	MorphChartEntry* bp1;
	MorphChartEntry* bp2;
	MorphChartEntry* fp1;
	MorphChartEntry* fb2;
	double alpha;
public:


};

#endif
