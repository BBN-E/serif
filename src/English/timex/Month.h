// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_MONTH_H
#define EN_MONTH_H

#include "English/timex/TimeUnit.h"
#include "English/timex/Year.h"

class Month : public TimeUnit {
public:
	static const int numMonths = 12;
	static const int monthDays[numMonths + 1];
	static const wstring monthNames[numMonths + 1];

	static const int numMonthRelatedValues = 10;
	static const wstring monthRelatedValues[numMonthRelatedValues];
	
	static int monthLength(int month, int year);
	static int month2Num(const wstring &str);
	static bool isMonth(const wstring &str);
	static std::pair<int,int> weekNum(int y, int m, int d);
	static bool isBefore(const Year *y1, const Month *m1, const Year *y2, const Month *m2);
	static bool isAfter(const Year *y1, const Month *m1, const Year *y2, const Month *m2);

	Month();
	Month(int val);
	Month(const wstring &month);
	Month* clone() const;
	bool isEmpty() const;
	Month* sub(int val) const;
	Month* add(int val) const;
	void clear();

	bool isWeekMode;
	
private:
	
	int* month2Range() const{
		int* r = _new int[2];
		if (!underspecified) {
			r[0] = r[1] = intVal;
		}
		else if (!strVal.compare(L"WI")) {
			r[0] = 0; r[1] = 2;
		} else if (!strVal.compare(L"SP")) {
			r[0] = 3; r[1] = 5;
		} else if (!strVal.compare(L"SU")) {
			r[0] = 6; r[1] = 8;
		} else if (!strVal.compare(L"FA")) {
			r[0] = 9; r[1] = 11;
		} else if (!strVal.compare(L"Q1")) {
			r[0] = 1; r[1] = 3;
		} else if (!strVal.compare(L"Q2")) {
			r[0] = 4; r[1] = 6;
		} else if (!strVal.compare(L"Q3")) {
			r[0] = 7; r[1] = 9;
		} else if (!strVal.compare(L"Q4")) {
			r[0] = 10; r[1] = 12;
		} else if (!strVal.compare(L"H1")) {
			r[0] = 1; r[1] = 6;
		} else if (!strVal.compare(L"H2")) {
			r[0] = 7; r[1] = 12;
		} else {
			r[0] = r[1] = 0;
		} 
		return r;
	}
};



#endif
