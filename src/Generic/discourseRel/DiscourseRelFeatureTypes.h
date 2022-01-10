// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef Discourse_Rel_FEATURE_TYPES_H
#define Discourse_Rel_FEATURE_TYPES_H

#include <boost/shared_ptr.hpp>

/** Home of the feature type class instances */
class DiscourseRelFeatureTypes {
public:
	/** Create and return a new DiscourseRelFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new DiscourseRelFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~DiscourseRelFeatureTypes() {}

	static void ensureBaseFeatureTypesInstantiated();
protected:
	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/discourseRel/en_DiscourseRelFeatureTypes.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "discourseRel/xx_DiscourseRelFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "discourseRel/xx_DiscourseRelFeatureTypes.h"
//#else
//	// default -- just does nothing
//	#include "Generic/discourseRel/xx_DiscourseRelFeatureTypes.h"
//#endif

#endif

