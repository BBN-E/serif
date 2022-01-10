// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/RelationModel.h"
#include "Generic/relations/xx_RelationModel.h"


boost::shared_ptr<RelationModel::Factory> &RelationModel::_factory() {
	static boost::shared_ptr<RelationModel::Factory> factory(new DefaultRelationModelFactory());
	return factory;
}
