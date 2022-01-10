#ifndef AR_DTCOREF_FEATURE_TYPES_H
#define AR_DTCOREF_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"


/** Home of the feature type class instances */
class ArabicDTCorefFeatureTypes : public DTCorefFeatureTypes {
private:
	friend class ArabicDTCorefFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
};

class ArabicDTCorefFeatureTypesFactory: public DTCorefFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ArabicDTCorefFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
