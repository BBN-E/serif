#ifndef CH_DTCOREF_FEATURE_TYPES_H
#define CH_DTCOREF_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"

/** Home of the feature type class instances */
class ChineseDTCorefFeatureTypes : public DTCorefFeatureTypes {
private:
	friend class ChineseDTCorefFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
};

class ChineseDTCorefFeatureTypesFactory: public DTCorefFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ChineseDTCorefFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
