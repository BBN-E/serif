// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MT_NORMALIZER_H
#define MT_NORMALIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"

#include <wchar.h>

class TokenSequence;

class MTNormalizer {
public:
	/** Create and return a new MTNormalizer. */
	static MTNormalizer *build() { return _factory()->build(); }
	/** Hook for registering new MTNormalizer factories */
	struct Factory { virtual MTNormalizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~MTNormalizer(){}

	void resetForNewSentence(){};
	virtual void normalize(TokenSequence* ts) = 0;

protected:
	MTNormalizer(){}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/normalizer/ar_MTNormalizer.h"
//#else
//	#include "Generic/normalizer/xx_MTNormalizer.h"
//#endif

#endif
