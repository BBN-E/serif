// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CBRNEWORDCONSTANTS_H
#define CBRNEWORDCONSTANTS_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"

class CBRNEWordConstants {
public:
	/** Create and return a new CBRNEWordConstants. */
	static bool isCBRNEReportedPreposition(Symbol word) { return _factory()->isCBRNEReportedPreposition(word); }
	/** Hook for registering new CBRNEWordConstants factories */
	struct Factory { virtual bool isCBRNEReportedPreposition(Symbol word) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

//public:
//	static bool isCBRNEReportedPreposition(Symbol word);
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/common/en_CBRNEWordConstants.h"
//#else
//	#include "Generic/common/xx_CBRNEWordConstants.h"
//#endif

#endif
