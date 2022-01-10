// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventModalityFeatureTypes.h"
#include "Generic/events/stat/xx_EventModalityFeatureTypes.h"




boost::shared_ptr<EventModalityFeatureTypes::Factory> &EventModalityFeatureTypes::_factory() {
	static boost::shared_ptr<EventModalityFeatureTypes::Factory> factory(new GenericEventModalityFeatureTypesFactory());
	return factory;
}

