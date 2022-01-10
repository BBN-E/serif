// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/results/ResultCollectionUtilities.h"
#include "Generic/results/xx_ResultCollectionUtilities.h"




boost::shared_ptr<ResultCollectionUtilities::Factory> &ResultCollectionUtilities::_factory() {
	static boost::shared_ptr<ResultCollectionUtilities::Factory> factory(new GenericResultCollectionUtilitiesFactory());
	return factory;
}

