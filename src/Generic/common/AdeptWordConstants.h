// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ADEPTWORDCONSTANTS_H
#define ADEPTWORDCONSTANTS_H

#include <boost/shared_ptr.hpp>

#include "common/Symbol.h"

class AdeptWordConstants {
public:
	/** Create and return a new AdeptWordConstants. */
	static bool isADEPTReportedPreposition(Symbol word) { return _factory()->isADEPTReportedPreposition(word); }
	/** Hook for registering new AdeptWordConstants factories */
	struct Factory { virtual bool isADEPTReportedPreposition(Symbol word) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

//public:
//	static bool isADEPTReportedPreposition(Symbol word);
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/common/en_AdeptWordConstants.h"
//#else
//	#include "common/xx_AdeptWordConstants.h"
//#endif

#endif
