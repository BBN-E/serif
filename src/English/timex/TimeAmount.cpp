// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/timex/TimeAmount.h"
#include "English/timex/Strings.h"
#include "Generic/common/Symbol.h"
#include <string>
#include <iostream>

using namespace std;

int TimeAmount::MAX_VALUE = 99999999;

TimeAmount::TimeAmount() {
	dYear = dMonth = dWeek = dDay = dHour = dMin = dSec = dDecade = dCent = dMil = 0;
	other = L"";
}

TimeAmount::~TimeAmount() { }

void TimeAmount::clear() {
	dYear = dMonth = dWeek = dDay = dHour = dMin = dSec = dDecade = dCent = dMil = 0;
	other = L"";	
}

TimeAmount* TimeAmount::clone() {
	TimeAmount *temp = _new TimeAmount();
	temp->dYear = dYear;
	temp->dMonth = dMonth;
	temp->dWeek = dWeek;
	temp->dDay = dDay;
	temp->dMin = dMin;
	temp->dHour = dHour;
	temp->dSec = dSec;
	temp->dDecade = dDecade;
	temp->dCent = dCent;
	temp->dMil = dMil;
	temp->other = other;
	return temp;
}

wstring TimeAmount::toString() {
	wstring result = L"";

	if (other.length() > 0) { 
		result += other;
	}

	else {
		if (dMil == TimeAmount::MAX_VALUE) result += L"XML";
		else if (dMil > 0) result += Strings::valueOf(dMil) + L"ML";
		if (dCent == TimeAmount::MAX_VALUE) result += L"XCE";
		else if (dCent > 0) result += Strings::valueOf(dCent) + L"CE";
		if (dDecade == TimeAmount::MAX_VALUE) result += L"XDE";
		else if (dDecade > 0) result += Strings::valueOf(dDecade) + L"DE";
		if (dWeek == TimeAmount::MAX_VALUE) result += L"XW";
		else if (dWeek > 0) result += Strings::valueOf(dWeek) + L"W";
		if (dYear == TimeAmount::MAX_VALUE) result += L"XY";
		else if (dYear > 0) result += Strings::valueOf(dYear) + L"Y"; 
		if (dMonth == TimeAmount::MAX_VALUE) result += L"XM";
		else if (dMonth > 0) result += Strings::valueOf(dMonth) + L"M";
		if (dDay == TimeAmount::MAX_VALUE) result += L"XD";
		else if (dDay > 0) result += Strings::valueOf(dDay) + L"D";

		if ((dHour > 0) || (dMin > 0) || (dSec > 0)) result += L"T";
		if (dHour == TimeAmount::MAX_VALUE) result += L"XH";
		else if (dHour > 0) result += Strings::valueOf(dHour) + L"H";
		if (dMin == TimeAmount::MAX_VALUE) result += L"XM";
		else if (dMin > 0) result += Strings::valueOf(dMin) + L"M";
		if (dSec == TimeAmount::MAX_VALUE) result += L"XS";
		else if (dSec > 0) result += Strings::valueOf(dSec) + L"S";
	}

	return result;
}

void TimeAmount::setOther(const wstring &s) {
	other = s;
}

void TimeAmount::set(const wchar_t *slot, int amt) {
	other = L"";
	if (!wcscmp(slot, L"L")) dMil = amt;
	if (!wcscmp(slot, L"C")) dCent = amt;
	if (!wcscmp(slot, L"E")) dDecade = amt;
	if (!wcscmp(slot, L"Y")) dYear = amt;
	if (!wcscmp(slot, L"M")) dMonth = amt;
	if (!wcscmp(slot, L"W")) dWeek = amt;
	if (!wcscmp(slot, L"D")) dDay = amt;
	if (!wcscmp(slot, L"H")) dHour = amt;
	if (!wcscmp(slot, L"N")) dMin = amt;
	if (!wcscmp(slot, L"S")) dSec = amt;
}

void TimeAmount::set(const wchar_t *slot, float amt) {
	other = L"";
	int base = (int)amt;
	float fraction = amt - base;
	set(slot, base);
	if (fraction > 0) {
		if (!wcscmp(slot, L"L")) set(L"Y", fraction * 1000);
		if (!wcscmp(slot, L"C")) set(L"Y", fraction * 100);
		if (!wcscmp(slot, L"E")) set(L"Y", fraction * 10);
		if (!wcscmp(slot, L"Y")) set(L"M", fraction * 12);
		if (!wcscmp(slot, L"M")) set(L"W", fraction * 4);
		if (!wcscmp(slot, L"W")) set(L"D", fraction * 7);
		if (!wcscmp(slot, L"D")) set(L"H", fraction * 24);
		if (!wcscmp(slot, L"H")) set(L"N", fraction * 60);
		if (!wcscmp(slot, L"N")) set(L"S", fraction * 60);
	}
}

int TimeAmount::get(const wchar_t *slot) {
	if (!wcscmp(slot, L"L")) return dMil;
	if (!wcscmp(slot, L"C")) return dCent;
	if (!wcscmp(slot, L"E")) return dDecade;
	if (!wcscmp(slot, L"Y")) return dYear;
	if (!wcscmp(slot, L"M")) return dMonth;
	if (!wcscmp(slot, L"W")) return dWeek;
	if (!wcscmp(slot, L"D")) return dDay;
	if (!wcscmp(slot, L"H")) return dHour;
	if (!wcscmp(slot, L"N")) return dMin;
	if (!wcscmp(slot, L"S")) return dSec;
	return 0;
}

int TimeAmount::compareTo(TimeAmount *a) {

	if (other.length() > 0 || a->other.length() > 0) return 0;
	int daySecs = 60 * 60 * 24;
	int yearSecs = (int)(daySecs * 365.25);
	int thisSeconds = (1 * dSec) + (60 * dMin) + (60 * 60 * dHour) +
		(daySecs * dDay) + (daySecs * 7 * dWeek) + (daySecs * 30 * dMonth) +
		(yearSecs * dYear) + (yearSecs * 10 * dDecade) + (yearSecs * 100 * dCent) +
		(yearSecs * 1000 * dMil);
	int aSeconds = (1 * a->dSec) + (60 * a->dMin) + (60 * 60 * a->dHour) +
		(daySecs * a->dDay) + (daySecs * 7 * a->dWeek) + (daySecs * 30 * a->dMonth) +
		(yearSecs * a->dYear) + (yearSecs * 10 * a->dDecade) + (yearSecs * 100 * a->dCent) +
		(yearSecs * 1000 * a->dMil);
	return (thisSeconds < aSeconds) ? -1 : ((thisSeconds > aSeconds) ? 1 : 0);
}
