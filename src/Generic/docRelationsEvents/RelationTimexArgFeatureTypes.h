// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TIMEX_ARG_FEATURE_TYPES_H
#define RELATION_TIMEX_ARG_FEATURE_TYPES_H



#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class RelationTimexArgFeatureTypes {
public:
	/** Create and return a new RelationTimexArgFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new RelationTimexArgFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~RelationTimexArgFeatureTypes() {}

//public:
//	static void ensureFeatureTypesInstantiated();
//private:
//	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#if defined(CHINESE_LANGUAGE)
//	#include "Chinese/docRelationsEvents/ch_RelationTimexArgFeatureTypes.h"
//#elif defined(ENGLISH_LANGUAGE)
//	#include "English/docRelationsEvents/en_RelationTimexArgFeatureTypes.h"
//#else
//	// default, doesn't do anything
//	#include "Generic/docRelationsEvents/xx_RelationTimexArgFeatureTypes.h"
//#endif


#endif

