// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_HEADFINDER_H
#define ar_HEADFINDER_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/trainers/Production.h"
class ArabicHeadFinder : public HeadFinder{
private:
	friend class ArabicHeadFinderFactory;

public:
	int get_head_index();
	//note - in the Arabic ArabicParser version of this 
	//Production was a public static member, make sure 
	//there aren't any problems
private:
	ArabicHeadFinder() {}
	//Since the Arabic ArabicHeadFinder works without paying attention
	//the constituent type, leave out constituent methods
	static int scan_from_left_to_right_by_set (Symbol* set, int size);
	static int scan_from_right_to_left_by_set (Symbol* set, int size);
	static int scan_from_left_to_right (Symbol* set, int size);
	static int scan_from_right_to_left (Symbol* set, int size);
	static int nonmatch_scan_from_left_to_right (Symbol* set, int size);
	//Adopt the Bikel based head rules that are constituent based
	static int findHeadADJP();
	static int findHeadADVP();
	static int findHeadNAC();
	static int findHeadNP();
	static int findHeadNX();
	static int findHeadPP();
	static int findHeadS();
	static int findHeadSBAR();
	static int findHeadSBARQ();
	static int findHeadSQ();
	static int findHeadVP();
	static int findHeadUCP();
	static int findHeadRRC();
	static int findHeadWHNP();




};

class ArabicHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new ArabicHeadFinder(); }
};

#endif
