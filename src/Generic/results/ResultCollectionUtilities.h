// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RESULT_COLLECTION_UTILITIES_H
#define RESULT_COLLECTION_UTILITIES_H

#include <boost/shared_ptr.hpp>

class EntitySet;
class Mention;

class ResultCollectionUtilities {
public:
	/** Create and return a new ResultCollectionUtilities. */
	static bool isPREmention(const EntitySet *entitySet, Mention *ment) { return _factory()->isPREmention(entitySet, ment); }
	/** Hook for registering new ResultCollectionUtilities factories */
	struct Factory { virtual bool isPREmention(const EntitySet *entitySet, Mention *ment) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~ResultCollectionUtilities() {}

//public:
//	static bool isPREmention(const EntitySet *entitySet, Mention *ment);
private:
	static boost::shared_ptr<Factory> &_factory();
};

// now we include a language-specific class which is called
// ResultCollectionUtilities and is a subclass of ResultCollectionUtilities
//#ifdef ENGLISH_LANGUAGE
//	#include "English/results/en_ResultCollectionUtilities.h"
//#elif defined ARABIC_LANGUAGE
//	#include "Arabic/results/ar_ResultCollectionUtilities.h"
//#else
//	#include "Generic/results/xx_ResultCollectionUtilities.h"
//#endif


#endif
