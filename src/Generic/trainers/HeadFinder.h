// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADFINDER_H
#define HEADFINDER_H

#include <boost/shared_ptr.hpp>

#include "Generic/trainers/Production.h"
#include <cstddef>
#include <string>


class HeadFinder {
public:
	/** Create and return a new HeadFinder. */
	static HeadFinder *build() { return _factory()->build(); }
	/** Hook for registering new HeadFinder factories */
	struct Factory { virtual HeadFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~HeadFinder() {}
	virtual int get_head_index() = 0;
	static Production production;
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/trainers/en_HeadFinder.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/trainers/ch_HeadFinder.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/trainers/ar_HeadFinder.h"
//#else
//	#include "Generic/trainers/xx_HeadFinder.h"
//
//#endif
#endif

