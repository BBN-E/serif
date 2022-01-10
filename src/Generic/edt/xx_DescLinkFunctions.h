// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESCLINKFUNCTIONS_H
#define xx_DESCLINKFUNCTIONS_H

#include "Generic/edt/DescLinkFunctions.h"

class GenericDescLinkFunctions : public DescLinkFunctions {
private:
	friend class GenericDescLinkFunctionsFactory;

private:
	static void defaultMsg(){
		SessionLogger::warn("unimplemented_class")<<"<<<<<<<<<<<<WARNING: Using an uninstantiated GenericDescLinkFunctions class >>>>>>>>>>\n";
	}
public:
	static void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) {
		defaultMsg();
		return;
	}


};

class GenericDescLinkFunctionsFactory: public DescLinkFunctions::Factory {
	virtual void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) {  GenericDescLinkFunctions::getEventContext(entSet, mention, entity, evt, maxSize); }
};


#endif
