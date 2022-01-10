// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PIDF_FEATURE_TYPE_SYMBOLS_H
#define PIDF_FEATURE_TYPE_SYMBOLS_H

#include "Generic/common/Symbol.h"


class FeatureTypesSymbols {
public:
	const static Symbol rareWordSym;
	const static Symbol outOfBoundSym;
	const static Symbol countEq0;
	const static Symbol countEq1To3;
	const static Symbol countEq4To8;
	const static Symbol countEq9OrMore;
	
	FeatureTypesSymbols() {}
};
#endif
