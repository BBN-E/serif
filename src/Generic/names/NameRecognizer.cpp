// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "names/NameRecognizer.h"
#include "names/xx_NameRecognizer.h"


boost::shared_ptr<NameRecognizer::Factory> &NameRecognizer::_factory() {
	static boost::shared_ptr<NameRecognizer::Factory> factory(new GenericNameRecognizerFactory());
	return factory;
}

