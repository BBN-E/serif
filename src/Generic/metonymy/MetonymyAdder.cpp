// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/metonymy/MetonymyAdder.h"
#include "Generic/metonymy/xx_MetonymyAdder.h"




boost::shared_ptr<MetonymyAdder::Factory> &MetonymyAdder::_factory() {
	static boost::shared_ptr<MetonymyAdder::Factory> factory(new GenericMetonymyAdderFactory());
	return factory;
}

