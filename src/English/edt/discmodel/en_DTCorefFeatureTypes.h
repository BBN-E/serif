#ifndef EN_DTCOREF_FEATURE_TYPES_H
#define EN_DTCOREF_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"

/** Home of the feature type class instances */
class EnglishDTCorefFeatureTypes : public DTCorefFeatureTypes {
private:
	friend class EnglishDTCorefFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
};

class EnglishDTCorefFeatureTypesFactory: public DTCorefFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishDTCorefFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
