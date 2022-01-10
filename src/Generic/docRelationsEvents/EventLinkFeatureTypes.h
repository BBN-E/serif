// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_LINK_FEATURE_TYPES_H
#define EVENT_LINK_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class EventLinkFeatureTypes {
public:
	/** Create and return a new EventLinkFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new EventLinkFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~EventLinkFeatureTypes() {}
//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/docRelationsEvents/en_EventLinkFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/docRelationsEvents/ch_EventLinkFeatureTypes.h"
//#else
//	// default, doesn't do anything
//	#include "Generic/docRelationsEvents/xx_EventLinkFeatureTypes.h"
//#endif


#endif

