#ifndef CH_EVENT_TRIGGER_FEATURE_TYPES_H
#define CH_EVENT_TRIGGER_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventTriggerFeatureTypes.h"

/** Home of the feature type class instances */
class ChineseEventTriggerFeatureTypes : public EventTriggerFeatureTypes {
private:
	friend class ChineseEventTriggerFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class ChineseEventTriggerFeatureTypesFactory: public EventTriggerFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ChineseEventTriggerFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
