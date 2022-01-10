// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/CBRNEWordConstants.h"
#include "Generic/common/xx_CBRNEWordConstants.h"




boost::shared_ptr<CBRNEWordConstants::Factory> &CBRNEWordConstants::_factory() {
	static boost::shared_ptr<CBRNEWordConstants::Factory> factory(new GenericCBRNEWordConstantsFactory());
	return factory;
}

