// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/FamilyRelationFinder.h"
#include "Generic/relations/xx_FamilyRelationFinder.h"


boost::shared_ptr<FamilyRelationFinder::Factory> &FamilyRelationFinder::_factory() {
	static boost::shared_ptr<FamilyRelationFinder::Factory> factory(new DefaultFamilyRelationFinderFactory());
	return factory;
}
