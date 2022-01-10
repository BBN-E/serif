// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_SEGMENT_H
#define ar_SEGMENT_H

#include "common/Symbol.h"
class Segment{
private:
	static const int PRON = 1;
	static const int PREP = 2;
	static const int PREP_OR_PART = 3;
	static const int CONJ = 4;

	Symbol _seg;
	int _pos_class;
	int _start;
	int _end;
	int _getPOSClass(Symbol seg, int place);


	
public:

	static const int PREFIX = 1;
	static const int STEM = 2;
	static const int SUFFIX = 3; 
 	static Symbol PREP_TAGS[1];
	static Symbol CONJ_TAGS[1];
	static Symbol PREP_OR_PART_TAGS[5];
	static Symbol PRON_TAGS[4];

	Segment(Symbol seg, int start, int type);
	Segment(const Segment* seg);
	~Segment();
	bool isValidPOS(Symbol pos);
	int getStart();
	int getEnd();
	void setEnd(int end);
	void setStart(int end);

	const Symbol* validPOSList();
	int posListSize();
	Symbol getText();


};

#endif
