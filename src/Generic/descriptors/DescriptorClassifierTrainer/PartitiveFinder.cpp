// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescriptorClassifierTrainer/PartitiveFinder.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/xx_PartitiveFinder.h"




boost::shared_ptr<PartitiveFinder::Factory> &PartitiveFinder::_factory() {
	static boost::shared_ptr<PartitiveFinder::Factory> factory(new GenericPartitiveFinderFactory());
	return factory;
}

