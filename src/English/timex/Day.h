// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_DAY_H
#define EN_DAY_H

#include "English/timex/TimeUnit.h"
#include <string>

class Day : public TimeUnit {
public:
	static const int numDays = 8;
	static const wstring daysOfWeek[numDays];

	static int dayOfWeek(int y, int m, int d);
	static int dayOfWeek2Num(const wstring &str);
	static bool isDayOfWeek(const wstring &str);

	bool isWeekMode;

	Day();
	Day(int val);
	Day(const wstring &day);
	Day(int val, bool weekMode);
	Day(const wstring &day, bool weekMode);
	Day* clone() const;
	void clear();
};



#endif
