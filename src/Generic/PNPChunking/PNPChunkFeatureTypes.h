// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PNPCHUNK_FEATURE_TYPES_H
#define PNPCHUNK_FEATURE_TYPES_H

/** Home of the feature type class instances */
class PNPChunkFeatureTypes {
public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

#endif

