// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_ADEPTWORDCONSTANTS_H
#define xx_ADEPTWORDCONSTANTS_H

#include "common/AdeptWordConstants.h"

// it displays an error message upon initialization
class GenericAdeptWordConstants : public AdeptWordConstants {
private:
	friend class GenericAdeptWordConstantsFactory;

public:
	static bool isADEPTReportedPreposition(Symbol word)
	{ return false; }

};

class GenericAdeptWordConstantsFactory: public AdeptWordConstants::Factory {
	virtual bool isADEPTReportedPreposition(Symbol word) {  return GenericAdeptWordConstants::isADEPTReportedPreposition(word); }
};

#endif

