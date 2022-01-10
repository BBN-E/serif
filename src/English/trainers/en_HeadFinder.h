// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_HEADFINDER_H
#define en_HEADFINDER_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/trainers/Production.h"


class EnglishHeadFinder : public HeadFinder{
private:
	friend class EnglishHeadFinderFactory;

public:
	int get_head_index();
private:	
	EnglishHeadFinder() {}
	static int findHeadADJP();
	static int findHeadADVP();
	static int findHeadCONJP();
	static int findHeadFRAG();
	static int findHeadINTJ();
	static int findHeadLST();
	static int findHeadNAC();
	static int findHeadNP();
	static int findHeadPP();
	static int findHeadPRN();
	static int findHeadPRT();
	static int findHeadQP();
	static int findHeadRRC();
	static int findHeadS();
	static int findHeadSBAR();
	static int findHeadSBARQ();
	static int findHeadSINV();
	static int findHeadSQ();
	static int findHeadUCP();
	static int findHeadVP();
	static int findHeadWHADJP();
	static int findHeadWHADVP();
	static int findHeadWHNP();
	static int findHeadWHPP();
	
	static int priority_scan_from_left_to_right (Symbol* set, int size);
	static int priority_scan_from_right_to_left (Symbol* set, int size);
	static int scan_from_left_to_right (Symbol* set, int size);
	static int scan_from_right_to_left (Symbol* set, int size);

};

class EnglishHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new EnglishHeadFinder(); }
};


#endif

