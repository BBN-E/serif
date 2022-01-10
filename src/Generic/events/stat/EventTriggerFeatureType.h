// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_FEATURE_TYPE_H
#define EVENT_TRIGGER_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class EventTriggerFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	EventTriggerFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
