// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/clutter/ClutterFilter.h"
#include "Generic/clutter/xx_ClutterFilter.h"


boost::shared_ptr<ClutterFilter::Factory> &ClutterFilter::_factory() {
	static boost::shared_ptr<ClutterFilter::Factory> factory(new DefaultClutterFilterFactory());
	return factory;
}

