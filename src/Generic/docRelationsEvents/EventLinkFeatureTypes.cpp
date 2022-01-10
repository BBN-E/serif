// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/EventLinkFeatureTypes.h"
#include "Generic/docRelationsEvents/xx_EventLinkFeatureTypes.h"




boost::shared_ptr<EventLinkFeatureTypes::Factory> &EventLinkFeatureTypes::_factory() {
	static boost::shared_ptr<EventLinkFeatureTypes::Factory> factory(new GenericEventLinkFeatureTypesFactory());
	return factory;
}

