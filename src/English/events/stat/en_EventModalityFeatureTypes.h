#ifndef EN_EVENT_MODALITY_FEATURE_TYPES_H
#define EN_EVENT_MODALITY_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/stat/EventModalityFeatureTypes.h"

/** Home of the feature type class instances */
class EnglishEventModalityFeatureTypes : public EventModalityFeatureTypes {
private:
	friend class EnglishEventModalityFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishEventModalityFeatureTypesFactory: public EventModalityFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishEventModalityFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
