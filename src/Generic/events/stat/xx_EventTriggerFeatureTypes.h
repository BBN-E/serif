#ifndef XX_EVENT_TRIGGER_FEATURE_TYPES_H
#define XX_EVENT_TRIGGER_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/** Home of the feature type class instances */
class GenericEventTriggerFeatureTypes : public EventTriggerFeatureTypes {
private:
	friend class GenericEventTriggerFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
private:
	static bool _instantiated;
};

class GenericEventTriggerFeatureTypesFactory: public EventTriggerFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericEventTriggerFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
