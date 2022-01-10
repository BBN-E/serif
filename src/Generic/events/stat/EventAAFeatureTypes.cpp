// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventAAFeatureTypes.h"
#include "Generic/events/stat/xx_EventAAFeatureTypes.h"




boost::shared_ptr<EventAAFeatureTypes::Factory> &EventAAFeatureTypes::_factory() {
	static boost::shared_ptr<EventAAFeatureTypes::Factory> factory(new GenericEventAAFeatureTypesFactory());
	return factory;
}

