#ifndef CH_P1DESC_FEATURE_TYPES_H
#define CH_P1DESC_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"


/** Home of the feature type class instances */
class ChineseP1DescFeatureTypes : public P1DescFeatureTypes {
private:
	friend class ChineseP1DescFeatureTypesFactory;
public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class ChineseP1DescFeatureTypesFactory: public P1DescFeatureTypes::Factory {

	virtual void ensureFeatureTypesInstantiated() { 
		ChineseP1DescFeatureTypes::ensureFeatureTypesInstantiated();
	}

};
#endif
