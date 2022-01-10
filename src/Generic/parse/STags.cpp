// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/STags.h"
#include "Generic/parse/xx_STags.h"

boost::shared_ptr<STags::STagsInstance> &STags::getInstance() {
	static boost::shared_ptr<STagsInstance> instance(_new STagsInstanceFor<GenericSTags>());
	return instance;
}

