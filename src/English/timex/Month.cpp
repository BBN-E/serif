// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"


#include "English/timex/Month.h"
#include <string>
#include <time.h>

#include "English/timex/Strings.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Year.h"

using namespace std; 

const wstring Month::monthRelatedValues[Month::numMonthRelatedValues] = {L"WI", L"SP", L"SU", L"FA", L"Q1", L"Q2", L"Q3", L"Q4", L"H1", L"H2"};
const int Month::monthDays[Month::numMonths + 1] = {31, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const wstring Month::monthNames[Month::numMonths + 1] = { L"", L"jan", L"feb", L"mar", L"apr", L"may", L"jun", L"jul", L"aug", L"sep", L"oct", L"nov", L"dec"}; 


Month::Month() {
	strVal = L"XX";
	isWeekMode = false;
}

void Month::clear() {
	strVal = L"XX";
	isWeekMode = false;
}

Month::Month(int val) {
	strVal = L"XX";
	isWeekMode = false;
	if ((val >= 1) && (val <= 12)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Month::Month(const wstring &month) {
	strVal = L"XX";
	isWeekMode = false;

	int val = Strings::parseInt(month);
	if ((val >= 1) && (val <= 12)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
	else 
	{
		strVal = L"XX";
		isWeekMode = false;
		if (Strings::startsWith(month, L"W") && (month.compare(L"WI"))) {  // treat month slot as a week value

			int w = Strings::parseInt(month.substr(1));
			if ((w >= 1) && (w <= 53)) {
				isWeekMode = true;
				strVal = month;
				intVal = w;
				underspecified = false;
			}
		}
		else {
			for (int i = 0; i < numMonthRelatedValues; i++) 
				if (!month.compare(monthRelatedValues[i]))
					strVal = month;
		}
	}
}

Month* Month::clone() const {
	Month* temp = _new Month();
	temp->intVal = intVal;
	temp->strVal = strVal;
	temp->underspecified = underspecified;
	temp->isWeekMode = isWeekMode;
	return temp;
}

bool Month::isEmpty() const {
	return (!strVal.compare(L"WXX") || !strVal.compare(L"XX"));
}

Month* Month::sub(int val) const {
	Month* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal -= val;
		size_t fill = temp->strVal.length();
		if (temp->isWeekMode) fill--;
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
		if (temp->isWeekMode) temp->strVal = L"W" + temp->strVal;
	}
	return temp;
}

Month* Month::add(int val) const {
	Month* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal += val;
		size_t fill = temp->strVal.length();
		if (temp->isWeekMode) fill--;
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
		if (temp->isWeekMode) temp->strVal = L"W" + temp->strVal;
	}
	return temp;
}

int Month::monthLength(int month, int year) {
	if (month < 0 || month > 12) 
		return 31;
	else if ((month == 2) && !Year::leapYearP(year)) 
		return 28;
	else return monthDays[month];
}

int Month::month2Num(const wstring &str) {
	for (int i = 1; i < numMonths + 1; i++)
		if (Strings::startsWith(str, monthNames[i])) return i;
	return 0;
}

bool Month::isMonth(const wstring &str) {
	for (int i = 1; i < numMonths + 1; i++)
		if (Strings::startsWith(str, monthNames[i])) return true;
	return false;
}

std::pair<int,int> Month::weekNum(int y, int m, int d) {
	if ((y < 0) || ((m < 1) || (m > 12)) || (d < 0)) {
		return std::make_pair(-1,-1);
	}

	boost::gregorian::date ourDate((unsigned short)y, (unsigned short)m, (unsigned short)d);
	int week_number = ourDate.week_number();

	if (m == 1 && week_number > 50)
		y--;

	if (m == 12 && week_number == 1)
		y++;

	return std::make_pair(y, week_number);
}

bool Month::isBefore(const Year *y1, const Month *m1, const Year *y2, const Month *m2) {
	if (m1->isEmpty() || m2->isEmpty()) return false;
	if (m1->isWeekMode == m2->isWeekMode) {
		if (!m1->underspecified && !m2->underspecified) 
			return m1->value() < m2->value();
		else {
			int* r1 = m1->month2Range();
			int* r2 = m2->month2Range();
			return (r1[1] < r2[0]);
		}
	} /* else if (m1->isWeekMode) {  // FIXME
	  int* r = m2->month2Range();
	  Calendar cal = new GregorianCalendar();
	  if (!y2->underspecified) cal.set(y2.value(), r[0]-1, 1);
	  else cal.set(cal.get(Calendar.YEAR), r[0]-1, 1);
	  return (m1.value() < cal.get(Calendar.WEEK_OF_YEAR));
	  } else if (m2.isWeekMode) {
	  int* r = m1.month2Range();
	  Calendar cal = new GregorianCalendar();
	  if (!y1.underspecified) cal.set(y1.value(), r[1]-1, monthDays[r[1]]);
	  else cal.set(cal.get(Calendar.YEAR), r[1]-1, monthDays[r[1]]);
	  return (cal.get(Calendar.WEEK_OF_YEAR) < m2.value());
	  } else */return false;
}

bool Month::isAfter(const Year *y1, const Month *m1, const Year *y2, const Month *m2) {
	if (m1->isEmpty() || m2->isEmpty()) return false;
	if (m1->isWeekMode == m2->isWeekMode) {
		if (!m1->underspecified && !m2->underspecified) 
			return m1->value() > m2->value();
		else {
			int* r1 = m1->month2Range();
			int* r2 = m2->month2Range();
			return (r1[0] > r2[1]);
		}
	} /* else if (m1.isWeekMode) {  // FIXME
	  int [] r = m2.month2Range();
	  Calendar cal = new GregorianCalendar();
	  if (!y2.underspecified) cal.set(y2.value(), r[0]-1, monthDays[r[0]]);
	  else cal.set(cal.get(Calendar.YEAR), r[0]-1, monthDays[r[0]]);
	  return (m1.value() > cal.get(Calendar.WEEK_OF_YEAR));
	  } else if (m2.isWeekMode) {
	  int [] r = m1.month2Range();
	  Calendar cal = new GregorianCalendar();
	  if (!y1.underspecified) cal.set(y1.value(), r[1]-1, 1);
	  else cal.set(cal.get(Calendar.YEAR), r[1]-1, 1);
	  return (cal.get(Calendar.WEEK_OF_YEAR) > m2.value());
	  } else */ return false;
}
