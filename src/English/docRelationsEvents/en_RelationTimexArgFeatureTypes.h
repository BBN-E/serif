#ifndef EN_RELATION_TIMEX_ARG_FEATURE_TYPES_H
#define EN_RELATION_TIMEX_ARG_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/RelationTimexArgFeatureTypes.h"


/** Home of the feature type class instances */
class EnglishRelationTimexArgFeatureTypes : public RelationTimexArgFeatureTypes {
private:
	friend class EnglishRelationTimexArgFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishRelationTimexArgFeatureTypesFactory: public RelationTimexArgFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishRelationTimexArgFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
