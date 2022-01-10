// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/RelationTimexArgFeatureTypes.h"
#include "Generic/docRelationsEvents/xx_RelationTimexArgFeatureTypes.h"




boost::shared_ptr<RelationTimexArgFeatureTypes::Factory> &RelationTimexArgFeatureTypes::_factory() {
	static boost::shared_ptr<RelationTimexArgFeatureTypes::Factory> factory(new GenericRelationTimexArgFeatureTypesFactory());
	return factory;
}

