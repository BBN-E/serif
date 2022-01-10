
// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/values/PatternEventValueRecognizer.h"
#include "Generic/values/xx_PatternEventValueRecognizer.h"


boost::shared_ptr<PatternEventValueRecognizer::Factory> &PatternEventValueRecognizer::_factory() {
	static boost::shared_ptr<PatternEventValueRecognizer::Factory> factory(new DefaultPatternEventValueRecognizerFactory());
	return factory;
}
