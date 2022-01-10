// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/RelationFinder.h"
#include "Generic/relations/xx_RelationFinder.h"


boost::shared_ptr<RelationFinder::Factory> &RelationFinder::_factory() {
	static boost::shared_ptr<RelationFinder::Factory> factory(new DefaultRelationFinderFactory());
	return factory;
}
