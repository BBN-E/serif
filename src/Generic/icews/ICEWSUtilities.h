// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_UTILITIES
#define ICEWS_UTILITIES

#include "Generic/theories/ActorMention.h"

class DocTheory;

class ICEWSUtilities {

public:
	static size_t getIcewsSentNo(ActorMention_ptr actorMention, const DocTheory* docTheory);

};



#endif
