// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MODALITY_FEATURE_TYPES_H
#define EVENT_MODALITY_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class EventModalityFeatureTypes {
public:
	/** Create and return a new EventModalityFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new EventModalityFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~EventModalityFeatureTypes() {}

//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/events/stat/en_EventModalityFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#includee "Generic/events/stat/xx_EventModalityFeatureTypes.h"	
//#else
//	// default, doesn't do anything
//	#include "Generic/events/stat/xx_EventModalityFeatureTypes.h"
//#endif


#endif

