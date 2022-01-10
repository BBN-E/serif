// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1RELATION_FEATURE_TYPE_H
#define P1RELATION_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class P1RelationFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	P1RelationFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
protected:
	static Symbol AT_LEAST_ONE;
	static Symbol AT_LEAST_TEN;
	static Symbol AT_LEAST_ONE_HUNDRED;
	static Symbol AT_LEAST_ONE_THOUSAND;

	static Symbol AT_LEAST_25_PERCENT;
	static Symbol AT_LEAST_50_PERCENT;
	static Symbol AT_LEAST_75_PERCENT;

	static Symbol MOST_FREQ_TYPE_MATCH;
};
#endif
