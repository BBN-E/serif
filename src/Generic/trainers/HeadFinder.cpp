// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/trainers/HeadFinder.h"
#include "Generic/trainers/xx_HeadFinder.h"
#include "Generic/trainers/Production.h"

Production HeadFinder::production;



boost::shared_ptr<HeadFinder::Factory> &HeadFinder::_factory() {
	static boost::shared_ptr<HeadFinder::Factory> factory(new GenericHeadFinderFactory());
	return factory;
}

