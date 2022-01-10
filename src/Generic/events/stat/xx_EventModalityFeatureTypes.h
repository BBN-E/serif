#ifndef XX_EVENT_MODALITY_FEATURE_TYPES_H
#define XX_EVENT_MODALITY_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventModalityFeatureTypes.h"

/** Home of the feature type class instances */
class GenericEventModalityFeatureTypes : public EventModalityFeatureTypes {
private:
	friend class GenericEventModalityFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
private:
	static bool _instantiated;
};

class GenericEventModalityFeatureTypesFactory: public EventModalityFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericEventModalityFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
