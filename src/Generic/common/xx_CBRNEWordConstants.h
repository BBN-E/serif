// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_CBRNEWORDCONSTANTS_H
#define xx_CBRNEWORDCONSTANTS_H

#include "Generic/common/CBRNEWordConstants.h"

// it displays an error message upon initialization
class GenericCBRNEWordConstants : public CBRNEWordConstants {
private:
	friend class GenericCBRNEWordConstantsFactory;

public:
	static bool isCBRNEReportedPreposition(Symbol word)
	{ return false; }

};

class GenericCBRNEWordConstantsFactory: public CBRNEWordConstants::Factory {
	virtual bool isCBRNEReportedPreposition(Symbol word) {  return GenericCBRNEWordConstants::isCBRNEReportedPreposition(word); }
};

#endif

