#ifndef CH_RELATION_TIMEX_ARG_FEATURE_TYPES_H
#define CH_RELATION_TIMEX_ARG_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/RelationTimexArgFeatureTypes.h"


/** Home of the feature type class instances */
class ChineseRelationTimexArgFeatureTypes : public RelationTimexArgFeatureTypes {
private:
	friend class ChineseRelationTimexArgFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class ChineseRelationTimexArgFeatureTypesFactory: public RelationTimexArgFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ChineseRelationTimexArgFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
