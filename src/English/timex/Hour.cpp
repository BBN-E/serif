// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/timex/Hour.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Strings.h"
#include <string>

using namespace std;

const wstring Hour::hourValues[Hour::numHours] = { L"MO", L"MI", L"AF", L"EV", L"NI", L"DT" };

Hour::Hour() {
	strVal = L"XX";
}
void Hour::clear() {
	strVal = L"XX";
}

Hour::Hour(int val) {
	strVal = L"XX";
	if ((val >= 0) && (val <= 24)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Hour::Hour(const wstring &hour) {
	strVal = L"XX";

	int val = Strings::parseInt(hour);
	if ((val >= 0) && (val <= 24)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	} else {
		for (int i = 0; i < numHours; i++) 
			if (!hour.compare(hourValues[i])) 
				strVal = hour;
	}	
}

Hour* Hour::clone() const {
	Hour* temp = _new Hour();
	temp->intVal = intVal;
	temp->strVal = strVal;
	temp->underspecified = underspecified;
	return temp;
}

bool Hour::isBefore(const Hour* h) const {
	if (!underspecified && !h->underspecified) {
		return (value() < h->value());
	} else {
		int r1[2];
		hour2Range(r1);
		int r2[2];
		h->hour2Range(r2);
		return (r1[1] < r2[0]);
	}
}

bool Hour::isAfter(const Hour* h) const {
	if (!underspecified && !h->underspecified) {
		return (value() > h->value());
	} else {
		int r1[2];
		hour2Range(r1);
		int r2[2];
		h->hour2Range(r2);
		return (r1[0] > r2[1]);
	}
}

// r should have 2 spaces
void Hour::hour2Range(int* r) const {
	if (!underspecified) {
		r[0] = r[1] = intVal;
	}
	else if (!strVal.compare(L"MO")) {
		r[0] = 1; r[1] = 11;
	} else if (!strVal.compare(L"MI")) {
		r[0] = 11; r[1] = 12;
	} else if (!strVal.compare(L"AF")) {
		r[0] = 13; r[1] = 16;
	} else if (!strVal.compare(L"EV")) {
		r[0] = 17; r[1] = 19;
	} else if (!strVal.compare(L"NI")) {
		r[0] = 20; r[1] = 12;
	} else if (!strVal.compare(L"DT")) {
		r[0] = 6; r[1] = 20; 
	} else {
		r[0] = r[1] = 0;
	}
}
