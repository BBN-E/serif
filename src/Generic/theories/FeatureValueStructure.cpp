// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/FeatureValueStructure.h"
#include "Generic/theories/xx_FeatureValueStructure.h"




boost::shared_ptr<FeatureValueStructure::Factory> &FeatureValueStructure::_factory() {
	static boost::shared_ptr<FeatureValueStructure::Factory> factory(new GenericFeatureValueStructureFactory());
	return factory;
}

