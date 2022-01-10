#ifndef XX_P1DESC_FEATURE_TYPES_H
#define XX_P1DESC_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/** Home of the feature type class instances */
class GenericP1DescFeatureTypes : public P1DescFeatureTypes {
private:
	friend class GenericP1DescFeatureTypesFactory;
public:
	static void ensureFeatureTypesInstantiated() {}
//private:
//	static bool _instantiated;
};

class GenericP1DescFeatureTypesFactory: public P1DescFeatureTypes::Factory {

	virtual void ensureFeatureTypesInstantiated() { 
		GenericP1DescFeatureTypes::ensureFeatureTypesInstantiated();
	}

};

#endif
