// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/relations/specific_relation_vector_models.h"
#include "Generic/relations/xx_FeatureVectorFilter.h"

boost::shared_ptr<TypeFeatureVectorModel::Factory> &TypeFeatureVectorModel::_factory() {
	static boost::shared_ptr<TypeFeatureVectorModel::Factory> factory
		(_new FactoryFor<TypeFeatureVectorModel, BackoffProbModel<2>, GenericFeatureVectorFilter>());
	return factory;
}

boost::shared_ptr<TypeB2PFeatureVectorModel::Factory> &TypeB2PFeatureVectorModel::_factory() {
	static boost::shared_ptr<TypeB2PFeatureVectorModel::Factory> factory
		(_new FactoryFor<TypeB2PFeatureVectorModel, BackoffProbModel<2>, GenericFeatureVectorFilter>());
	return factory;
}
