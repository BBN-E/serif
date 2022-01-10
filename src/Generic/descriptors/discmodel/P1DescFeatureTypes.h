// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1_DESC_FEATURE_TYPES_H
#define P1_DESC_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class P1DescFeatureTypes {
public:
	/** Create and return a new P1DescFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new P1DescFeatureTypes factories. */
	struct Factory { 
		virtual void ensureFeatureTypesInstantiated() = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~P1DescFeatureTypes() {}

private:
	static boost::shared_ptr<Factory> &_factory();

//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/discmodel/en_P1DescFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/descriptors/discmodel/ch_P1DescFeatureTypes.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/descriptors/discmodel/ar_P1DescFeatureTypes.h"
//
//#else
//	// default -- just does nothing
//	#include "Generic/descriptors/discmodel/xx_P1DescFeatureTypes.h"
//#endif

#endif

