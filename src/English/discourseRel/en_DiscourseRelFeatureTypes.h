#ifndef EN_DISCOURSE_REL_FEATURE_TYPES_H
#define EN_DISCOURSE_REL_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/discourseRel/DiscourseRelFeatureTypes.h"


/** Home of the feature type class instances */
class EnglishDiscourseRelFeatureTypes : public DiscourseRelFeatureTypes {
private:
	friend class EnglishDiscourseRelFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishDiscourseRelFeatureTypesFactory: public DiscourseRelFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishDiscourseRelFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
