#ifndef EN_EVENT_AAFEATURE_TYPES_H
#define EN_EVENT_AAFEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventAAFeatureTypes.h"

/** Home of the feature type class instances */
class EnglishEventAAFeatureTypes : public EventAAFeatureTypes {
private:
	friend class EnglishEventAAFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishEventAAFeatureTypesFactory: public EventAAFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishEventAAFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
