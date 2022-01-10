// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/generics/GenericsFilter.h"
#include "Generic/generics/xx_GenericsFilter.h"


boost::shared_ptr<GenericsFilter::Factory> &GenericsFilter::_factory() {
	static boost::shared_ptr<GenericsFilter::Factory> factory(new DefaultGenericsFilterFactory());
	return factory;
}
