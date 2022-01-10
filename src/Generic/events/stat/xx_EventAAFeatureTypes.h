#ifndef XX_EVENT_AAFEATURE_TYPES_H
#define XX_EVENT_AAFEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventAAFeatureTypes.h"

/** Home of the feature type class instances */
class GenericEventAAFeatureTypes : public EventAAFeatureTypes {
private:
	friend class GenericEventAAFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
//private:
//	static bool _instantiated;
};

class GenericEventAAFeatureTypesFactory: public EventAAFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericEventAAFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
