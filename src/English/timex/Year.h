// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_YEAR_H
#define EN_YEAR_H

#include "English/timex/TimeUnit.h"

using namespace std;

class Year : public TimeUnit {
public:
	static bool leapYearP(int yr);
	static wstring canonicalizeYear(int n);

	Year();
	Year(int val);
	Year(const wstring& year);
	Year* clone() const;
	Year* sub(int val) const;
	Year* add(int val) const;
	void clear();
};

#endif
