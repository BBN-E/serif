// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GROUP_FN_GUESSER_H
#define GROUP_FN_GUESSER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

class GroupFnGuesser {
public:
	/** Create and return a new GroupFnGuesser. */
	static int isPlural(const Mention *mention) { return _factory()->isPlural(mention); }
	/** Hook for registering new GroupFnGuesser factories */
	struct Factory { virtual int isPlural(const Mention *mention) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~GroupFnGuesser() {}

	enum {SINGULAR, PLURAL, UNKNOWN};
//	static int isPlural(const Mention *mention);
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/eeml/en_GroupFnGuesser.h"
//#else
//	#include "Generic/eeml/xx_GroupFnGuesser.h"
//#endif



#endif
