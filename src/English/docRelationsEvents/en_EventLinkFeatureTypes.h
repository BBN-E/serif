#ifndef EN_EVENT_LINK_FEATURE_TYPES_H
#define EN_EVENT_LINK_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/EventLinkFeatureTypes.h"


/** Home of the feature type class instances */
class EnglishEventLinkFeatureTypes : public EventLinkFeatureTypes {
private:
	friend class EnglishEventLinkFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class EnglishEventLinkFeatureTypesFactory: public EventLinkFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  EnglishEventLinkFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
