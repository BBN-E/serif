// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/values/TemporalNormalizer.h"
#include "Generic/values/xx_TemporalNormalizer.h"




boost::shared_ptr<TemporalNormalizer::Factory> &TemporalNormalizer::_factory() {
	static boost::shared_ptr<TemporalNormalizer::Factory> factory(new GenericTemporalNormalizerFactory());
	return factory;
}

