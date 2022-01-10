// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PIDF_FEATURE_TYPES_H
#define PIDF_FEATURE_TYPES_H

/** Home of the feature type class instances */
class PIdFFeatureTypes {
public:
	//const static Symbol outOfBoundSym;
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

#endif

