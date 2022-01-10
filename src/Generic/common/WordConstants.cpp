// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/xx_WordConstants.h"

boost::shared_ptr<WordConstants::WordConstantsInstance> &WordConstants::getInstance() {
	static boost::shared_ptr<WordConstantsInstance> instance(_new WordConstantsInstanceFor<GenericWordConstants>());
	return instance;
}

