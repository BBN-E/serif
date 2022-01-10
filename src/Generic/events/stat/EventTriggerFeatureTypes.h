// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_FEATURE_TYPES_H
#define EVENT_TRIGGER_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class EventTriggerFeatureTypes {
public:
	/** Create and return a new EventTriggerFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new EventTriggerFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~EventTriggerFeatureTypes() {}
//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/events/stat/en_EventTriggerFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/events/stat/ch_EventTriggerFeatureTypes.h"
//#else
//	// default, doesn't do anything
//	#include "Generic/events/stat/xx_EventTriggerFeatureTypes.h"
//#endif


#endif

