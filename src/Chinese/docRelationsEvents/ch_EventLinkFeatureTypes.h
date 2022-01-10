#ifndef CH_EVENT_LINK_FEATURE_TYPES_H
#define CH_EVENT_LINK_FEATURE_TYPES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/docRelationsEvents/EventLinkFeatureTypes.h"

/** Home of the feature type class instances */
class ChineseEventLinkFeatureTypes : public EventLinkFeatureTypes {
private:
	friend class ChineseEventLinkFeatureTypesFactory;

public:
	static void ensureFeatureTypesInstantiated();
private:
	static bool _instantiated;
};

class ChineseEventLinkFeatureTypesFactory: public EventLinkFeatureTypes::Factory {
	virtual void ensureFeatureTypesInstantiated() {  ChineseEventLinkFeatureTypes::ensureFeatureTypesInstantiated(); }
};



#endif
