// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"


#include <string>
#include <cstdlib>
#include "English/timex/TimeUnit.h"
#include "English/timex/Strings.h"

using namespace std;

TimeUnit::TimeUnit() {
	strVal = L"";
	intVal = -1;
	underspecified = true;
}

void TimeUnit::clear() {
	strVal = L"";
	intVal = -1;
	underspecified = true;
}


wstring TimeUnit::toString() const {
	return strVal;
}

int TimeUnit::value() const {
	return intVal;
}

bool TimeUnit::isEmpty() const {
	for (size_t i = 0; i < strVal.length(); i++)
		if (strVal[i] != 'X') return false;
	return true;
} 

int TimeUnit::compareTo(const TimeUnit *t) const {
	if (value() < t->value()) return -1;
	if (value() > t->value()) return 1;
	else return 0;
}

TimeUnit* TimeUnit::sub(int val) const {
	TimeUnit* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal -= val;
		size_t fill = temp->strVal.length();
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
	}
	return temp;
}

TimeUnit* TimeUnit::add(int val) const {
	TimeUnit* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal += val;
		size_t fill = temp->strVal.length();
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
	}
	return temp;
}
