// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1RELATION_FEATURE_TYPES_H
#define P1RELATION_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class P1RelationFeatureTypes {
public:

	/** Create and return a new P1RelationFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new P1RelationFeatureTypes factories. */
	struct Factory { 
		virtual void ensureFeatureTypesInstantiated() = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

//	virtual void ensureFeatureTypesInstantiated() = 0;

	virtual ~P1RelationFeatureTypes() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/relations/discmodel/en_P1RelationFeatureTypes.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/relations/discmodel/ar_P1RelationFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/relations/discmodel/ch_P1RelationFeatureTypes.h"
//#else
//	// default -- just does nothing
//	#include "Generic/relations/discmodel/xx_P1RelationFeatureTypes.h"
//#endif

#endif

