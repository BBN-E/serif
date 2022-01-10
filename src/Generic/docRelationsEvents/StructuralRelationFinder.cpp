// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/StructuralRelationFinder.h"
#include "Generic/docRelationsEvents/xx_StructuralRelationFinder.h"




boost::shared_ptr<StructuralRelationFinder::Factory> &StructuralRelationFinder::_factory() {
	static boost::shared_ptr<StructuralRelationFinder::Factory> factory(new GenericStructuralRelationFinderFactory());
	return factory;
}

