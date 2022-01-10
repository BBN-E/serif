// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/xx_P1RelationFeatureTypes.h"

boost::shared_ptr<P1RelationFeatureTypes::Factory> &P1RelationFeatureTypes::_factory() {
	static boost::shared_ptr<P1RelationFeatureTypes::Factory> factory(new DefaultP1RelationFeatureTypesFactory());
	return factory;
}
