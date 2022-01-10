// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/EventLinker.h"
#include "Generic/events/xx_EventLinker.h"




boost::shared_ptr<EventLinker::Factory> &EventLinker::_factory() {
	static boost::shared_ptr<EventLinker::Factory> factory(new GenericEventLinkerFactory());
	return factory;
}

