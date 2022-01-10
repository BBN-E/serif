#ifndef XX_DTCOREF_FEATURE_TYPES_H
#define XX_DTCOREF_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"

/** Home of the feature type class instances */
class GenericDTCorefFeatureTypes : public DTCorefFeatureTypes {
private:
	friend class GenericDTCorefFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
};

class GenericDTCorefFeatureTypesFactory: public DTCorefFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericDTCorefFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
