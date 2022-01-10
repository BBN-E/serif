// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_COREF_FEATURE_TYPES_H
#define DT_COREF_FEATURE_TYPES_H

#include "Generic/theories/Mention.h"

#include <boost/shared_ptr.hpp>

class DTFeatureTypeSet;

/** Home of the feature type class instances */
class DTCorefFeatureTypes {
public:
	/** Create and return a new DTCorefFeatureTypes. */
	static void ensureFeatureTypesInstantiated() { _factory()->ensureFeatureTypesInstantiated(); }
	/** Hook for registering new DTCorefFeatureTypes factories */
	struct Factory { virtual void ensureFeatureTypesInstantiated() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~DTCorefFeatureTypes() {}

	static void ensureBaseFeatureTypesInstantiated();
	static DTFeatureTypeSet* makeNoneFeatureTypeSet(Mention::Type type);
protected:
	static bool _instantiated;
private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/edt/discmodel/en_DTCorefFeatureTypes.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/edt/discmodel/ar_DTCorefFeatureTypes.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/edt/discmodel/ch_DTCorefFeatureTypes.h"
//#else
//	// default -- just does nothing
//	#include "Generic/edt/discmodel/xx_DTCorefFeatureTypes.h"
//#endif

#endif

