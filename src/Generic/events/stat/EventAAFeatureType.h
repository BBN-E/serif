// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_AA_FEATURE_TYPE_H
#define EVENT_AA_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class EventAAFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	EventAAFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
