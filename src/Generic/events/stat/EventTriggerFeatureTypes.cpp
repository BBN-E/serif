// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventTriggerFeatureTypes.h"
#include "Generic/events/stat/xx_EventTriggerFeatureTypes.h"




boost::shared_ptr<EventTriggerFeatureTypes::Factory> &EventTriggerFeatureTypes::_factory() {
	static boost::shared_ptr<EventTriggerFeatureTypes::Factory> factory(new GenericEventTriggerFeatureTypesFactory());
	return factory;
}

