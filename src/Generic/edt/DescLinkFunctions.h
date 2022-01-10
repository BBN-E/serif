// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCLINKFUNCTIONS_H
#define DESCLINKFUNCTIONS_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Entity.h"
#include "Generic/edt/CountsTable.h"
#include "Generic/maxent/OldMaxEntEvent.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"

class DescLinkFunctions {
public:
	/** Create and return a new DescLinkFunctions. */
	static void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) { _factory()->getEventContext(entSet, mention, entity, evt, maxSize); }
	/** Hook for registering new DescLinkFunctions factories */
	struct Factory { virtual void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


//	static void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize);
	static Symbol OC_LINK;
	static Symbol OC_NO_LINK;
	static Symbol HEAD_WORD_MATCH;

	virtual ~DescLinkFunctions() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef CHINESE_LANGUAGE
//	#include "Chinese/edt/ch_DescLinkFunctions.h"
//#elif defined(ENGLISH_LANGUAGE)
//	#include "English/edt/en_DescLinkFunctions.h"
//#else
//	#include "Generic/edt/xx_DescLinkFunctions.h"
//#endif
#endif
