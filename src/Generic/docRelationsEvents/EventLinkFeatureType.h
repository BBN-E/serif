// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_LINK_FEATURE_TYPE_H
#define EVENT_LINK_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class EventLinkFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	EventLinkFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
