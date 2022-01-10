#ifndef EN_EVENT_TRIGGER_FEATURE_TYPES_H
#define EN_EVENT_TRIGGER_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventTriggerFeatureTypes.h"

/** Home of the feature type class instances */
class EnglishEventTriggerFeatureTypes : public EventTriggerFeatureTypes {
private:
	friend class EnglishEventTriggerFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishEventTriggerFeatureTypesFactory: public EventTriggerFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishEventTriggerFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
