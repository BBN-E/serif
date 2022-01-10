// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/AdeptWordConstants.h"
#include "Generic/common/xx_AdeptWordConstants.h"




boost::shared_ptr<AdeptWordConstants::Factory> &AdeptWordConstants::_factory() {
	static boost::shared_ptr<AdeptWordConstants::Factory> factory(new GenericAdeptWordConstantsFactory());
	return factory;
}

