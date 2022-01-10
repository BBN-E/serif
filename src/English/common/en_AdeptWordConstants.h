// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_ADEPT_WORD_CONSTANTS_H
#define en_ADEPT_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/AdeptWordConstants.h"

// place static symbols here that may be used by other classes 
// in various English specific, Adept specific areas.

class EnglishAdeptWordConstants : public AdeptWordConstants {
private:
	friend class EnglishAdeptWordConstantsFactory;

public:

	//for the ADEPT task, we report prepositions as part of "unknown" relations.
	//  the following are the prepositions we report

	static Symbol OF;
	static Symbol IN;
	static Symbol FOR;
	static Symbol ON;
	static Symbol AT;
	static Symbol WITH;
	static Symbol BY;
	static Symbol AS;
	static Symbol FROM;
	static Symbol ABOUT;
	static Symbol INTO;
	static Symbol AFTER;
	static Symbol OVER;
	static Symbol SINCE;
	static Symbol UNDER;
	static Symbol LIKE;
	static Symbol BEFORE;
	static Symbol UNTIL;
	static Symbol DURING;
	static Symbol THROUGH;
	static Symbol AGAINST;
	static Symbol BETWEEN;
	static Symbol WITHOUT;
	static Symbol BELOW;

	static bool isADEPTReportedPreposition(Symbol word);
};

class EnglishAdeptWordConstantsFactory: public AdeptWordConstants::Factory {
	virtual bool isADEPTReportedPreposition(Symbol word) {  return EnglishAdeptWordConstants::isADEPTReportedPreposition(word); }
};


#endif


