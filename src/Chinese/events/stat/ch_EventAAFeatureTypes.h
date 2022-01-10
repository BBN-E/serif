#ifndef CH_EVENT_AAFEATURE_TYPES_H
#define CH_EVENT_AAFEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventAAFeatureTypes.h"


/** Home of the feature type class instances */
class ChineseEventAAFeatureTypes : public EventAAFeatureTypes {
private:
	friend class ChineseEventAAFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class ChineseEventAAFeatureTypesFactory: public EventAAFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ChineseEventAAFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
