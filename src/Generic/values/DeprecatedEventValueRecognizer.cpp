
// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/values/DeprecatedEventValueRecognizer.h"
#include "Generic/values/xx_DeprecatedEventValueRecognizer.h"


boost::shared_ptr<DeprecatedEventValueRecognizer::Factory> &DeprecatedEventValueRecognizer::_factory() {
	static boost::shared_ptr<DeprecatedEventValueRecognizer::Factory> factory(new DefaultDeprecatedEventValueRecognizerFactory());
	return factory;
}
