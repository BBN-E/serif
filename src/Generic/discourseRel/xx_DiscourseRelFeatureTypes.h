#ifndef XX_DISCOURSE_REL_FEATURE_TYPES_H
#define XX_DISCOURSE_REL_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/discourseRel/DiscourseRelFeatureTypes.h"


/** Home of the feature type class instances */
class GenericDiscourseRelFeatureTypes : public DiscourseRelFeatureTypes {
private:
	friend class GenericDiscourseRelFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {};
private:
	static bool _instantiated;
};

class GenericDiscourseRelFeatureTypesFactory: public DiscourseRelFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericDiscourseRelFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
