// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/DescLinker.h"
#include "Generic/edt/xx_DescLinker.h"




boost::shared_ptr<DescLinker::Factory> &DescLinker::_factory() {
	static boost::shared_ptr<DescLinker::Factory> factory(new GenericDescLinkerFactory());
	return factory;
}

