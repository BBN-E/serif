#ifndef EN_P1DESC_FEATURE_TYPES_H
#define EN_P1DESC_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"


/** Home of the feature type class instances */
class EnglishP1DescFeatureTypes : public P1DescFeatureTypes {
private:
	friend class ArabicP1DescFeatureTypesFactory;
public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishP1DescFeatureTypesFactory: public P1DescFeatureTypes::Factory {

	virtual void ensureFeatureTypesInstantiated() { 
		EnglishP1DescFeatureTypes::ensureFeatureTypesInstantiated();
	}

};

#endif
