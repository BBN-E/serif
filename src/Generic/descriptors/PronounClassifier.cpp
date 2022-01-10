// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/descriptors/xx_PronounClassifier.h"




boost::shared_ptr<PronounClassifier::Factory> &PronounClassifier::_factory() {
	static boost::shared_ptr<PronounClassifier::Factory> factory(new DefaultPronounClassifierFactory());
	return factory;
}

