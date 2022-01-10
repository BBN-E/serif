// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TIME_ARG_FEATURE_TYPE_H
#define RELATION_TIME_ARG_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class RelationTimexArgFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	RelationTimexArgFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
