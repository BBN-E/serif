// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COREFDOCUMENT_H
#define COREFDOCUMENT_H

#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/trainers/CorefItem.h"

class CorefDocument {
public:
	CorefDocument(): parses(25), corefItems(50) { }
	~CorefDocument() {
		while(parses.length() > 0) delete parses.removeLast();
		while(corefItems.length() > 0) delete corefItems.removeLast();
	}
		
	Symbol documentName;
	GrowableArray <Parse *> parses;
	GrowableArray <CorefItem *> corefItems;
};

#endif
