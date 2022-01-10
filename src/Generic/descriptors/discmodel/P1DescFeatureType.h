// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1DESC_FEATURE_TYPE_H
#define P1DESC_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class P1DescFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	P1DescFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
