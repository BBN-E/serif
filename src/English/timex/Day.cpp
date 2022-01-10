// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/timex/Day.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Strings.h"

using namespace std;

const wstring Day::daysOfWeek[] = {L"", L"mon", L"tue", L"wed", L"thu", L"fri", L"sat", L"sun" };

// Returns 1-7 representing Monday .. Sunday
int Day::dayOfWeek(int year, int month, int day) {
	if ((year < 0) || ((month < 0) || (month > 12)) || (day < 0) || (day > 31)) {
		return -1;
	}
	int a = (14 - month) / 12;
	year -= a;
	month += (12 * a) - 2;
	int answer = (day + year + (year / 4) - (year / 100) +
		(year / 400) + (31 * month / 12)) % 7; // equals 0 - 6 representing Sunday ... Saturday
	return (answer ? answer : 7);  // because we want Sunday to be 7, not 0
}

int Day::dayOfWeek2Num(const wstring &str) {
	for (int i = 1; i < numDays; i++) 
		if (Strings::startsWith(str, daysOfWeek[i])) return i;
	return 0;
}

bool Day::isDayOfWeek(const wstring &str) {
	for (int i = 1; i < numDays; i++) 
		if (Strings::startsWith(str, daysOfWeek[i])) return true;
	return false;
}

void Day::clear() {
	strVal = L"XX";
	isWeekMode = false;
}

Day::Day() {
	strVal = L"XX";
	isWeekMode = false;
}

Day::Day(int val) {
	strVal = L"XX";
	isWeekMode = false;
	if ((val >= 1) && (val <= 31)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Day::Day(const wstring &day) {
	strVal = L"XX";
	isWeekMode = false;
	int val = Strings::parseInt(day);
	if ((val >= 1) && (val <= 31)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Day::Day(int val, bool weekMode) {
	strVal = L"XX";
	isWeekMode = false;
	if (weekMode) {
		isWeekMode = true;
		if ((val >= 1) && (val <= 7)) {
			intVal = val;
			strVal = Strings::valueOf(val);
			underspecified = false;
		}
		else strVal = L"X";
	}
	else if ((val >= 1) && (val <= 31)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Day::Day(const wstring &day, bool weekMode) {
	strVal = L"XX";
	isWeekMode = false;
	if (weekMode) {
		isWeekMode = true;
		int val = Strings::parseInt(day);
		if ((val >= 1) && (val <= 7)) {
			intVal = val;
			strVal = Strings::valueOf(val);
			underspecified = false;
		}
		else {
			if (!day.compare(L"WE")) strVal = day;
			else strVal = L"X"; 
		}
	} else {
		int val = Strings::parseInt(day);
		if ((val >= 1) && (val <= 31)) {
			intVal = val;
			strVal = Strings::valueOf(val);
			if (strVal.length() < 2) strVal = L"0" + strVal;
			underspecified = false;
		}
	}
}

Day* Day::clone() const {
	Day* temp = _new Day();
	temp->intVal = intVal;
	temp->strVal = strVal;
	temp->underspecified = underspecified;
	temp->isWeekMode = isWeekMode;
	return temp;
}
