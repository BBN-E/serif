#ifndef XX_EVENT_LINK_FEATURE_TYPES_H
#define XX_EVENT_LINK_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/EventLinkFeatureTypes.h"


/** Home of the feature type class instances */
class GenericEventLinkFeatureTypes : public EventLinkFeatureTypes {
private:
	friend class GenericEventLinkFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated() {}
//private:
//	static bool _instantiated;
};

class GenericEventLinkFeatureTypesFactory: public EventLinkFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  GenericEventLinkFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
