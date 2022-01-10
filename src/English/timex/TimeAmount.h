// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_TIME_AMOUNT_H
#define EN_TIME_AMOUNT_H

#include <string>
using namespace std;

class TimeAmount
{
private:

	int dYear;
	int dMonth;
	int dWeek;
	int dDay;
	int dHour;
	int dMin;
	int dSec;
	int dDecade;
	int dCent;
	int dMil;
	wstring other;

public:
	
	TimeAmount();
	~TimeAmount();
	TimeAmount* clone();
	wstring toString();
	void setOther(const wstring &str);
	void set(const wchar_t *slot, int amt);
	void set(const wchar_t *slot, float amt);
	int get(const wchar_t *slot);
	int compareTo(TimeAmount *ta);

	static int MAX_VALUE;
	void clear();

};

#endif

