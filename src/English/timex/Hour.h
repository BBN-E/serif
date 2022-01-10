// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_HOUR_H
#define EN_HOUR_H

#include "English/timex/TimeUnit.h"
#include <string>

class Hour : public TimeUnit {
public:
	static const int numHours = 6;
	static const wstring hourValues[numHours];
	Hour();
	Hour(int val);
	Hour(const wstring &hour);
	Hour* clone() const;
	bool isBefore(const Hour* h) const;
	bool isAfter(const Hour* h) const;
	void clear();

private:
	// r should have 2 spaces
	void hour2Range(int* r) const;
};



#endif
