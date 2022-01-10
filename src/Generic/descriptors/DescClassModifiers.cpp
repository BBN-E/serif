// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescClassModifiers.h"
#include "Generic/descriptors/xx_DescClassModifiers.h"




boost::shared_ptr<DescClassModifiers::Factory> &DescClassModifiers::_factory() {
	static boost::shared_ptr<DescClassModifiers::Factory> factory(new DefaultDescClassModifiersFactory());
	return factory;
}

