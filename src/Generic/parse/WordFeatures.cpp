// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/WordFeatures.h"
#include "Generic/parse/xx_WordFeatures.h"




boost::shared_ptr<WordFeatures::Factory> &WordFeatures::_factory() {
	static boost::shared_ptr<WordFeatures::Factory> factory(new GenericWordFeaturesFactory());
	return factory;
}

