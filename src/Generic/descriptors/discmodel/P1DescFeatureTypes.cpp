// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/xx_P1DescFeatureTypes.h"

boost::shared_ptr<P1DescFeatureTypes::Factory> &P1DescFeatureTypes::_factory() {
	static boost::shared_ptr<P1DescFeatureTypes::Factory> factory(new GenericP1DescFeatureTypesFactory());
	return factory;
}
