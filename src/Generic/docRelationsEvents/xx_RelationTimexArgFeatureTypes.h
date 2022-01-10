#ifndef XX_RELATION_TIMEX_ARG_FEATURE_TYPES_H
#define XX_RELATION_TIMEX_ARG_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/RelationTimexArgFeatureTypes.h"


/** Home of the feature type class instances */
class GenericRelationTimexArgFeatureTypes : public RelationTimexArgFeatureTypes {
private:
	friend class GenericRelationTimexArgFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
//private:
//	static bool _instantiated;
};

class GenericRelationTimexArgFeatureTypesFactory: public RelationTimexArgFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericRelationTimexArgFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
