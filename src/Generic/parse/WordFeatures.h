// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORD_FEATURES_H
#define WORD_FEATURES_H

#include <boost/shared_ptr.hpp>

#include <string>
//#include <cstdio>
#include "Generic/common/Symbol.h"

class WordFeatures {
public:
	/** Create and return a new WordFeatures. */
	static WordFeatures *build() { return _factory()->build(); }
	/** Hook for registering new WordFeatures factories */
	struct Factory { virtual WordFeatures *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~WordFeatures() {}
    virtual Symbol features(Symbol wordSym, bool firstWord) const = 0;
    virtual Symbol reducedFeatures(Symbol wordSym, bool firstWord) const = 0;

protected:
    WordFeatures() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/parse/en_WordFeatures.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/parse/ch_WordFeatures.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/parse/ar_WordFeatures.h"
//#else
//	#include "Generic/parse/xx_WordFeatures.h"
//#endif


#endif
