// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MODALITY_FEATURE_TYPE_H
#define EVENT_MODALITY_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class EventModalityFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	EventModalityFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}
};
#endif
