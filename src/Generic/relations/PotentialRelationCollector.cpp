// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/PotentialRelationCollector.h"
#include "Generic/relations/xx_PotentialRelationCollector.h"




boost::shared_ptr<PotentialRelationCollector::Factory> &PotentialRelationCollector::_factory() {
	static boost::shared_ptr<PotentialRelationCollector::Factory> factory(new GenericPotentialRelationCollectorFactory());
	return factory;
}

