// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/normalizer/MTNormalizer.h"
#include "Generic/normalizer/xx_MTNormalizer.h"


boost::shared_ptr<MTNormalizer::Factory> &MTNormalizer::_factory() {
	static boost::shared_ptr<MTNormalizer::Factory> factory(new GenericMTNormalizerFactory());
	return factory;
}

