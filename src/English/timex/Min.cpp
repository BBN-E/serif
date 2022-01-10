// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"


#include "English/timex/Min.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Strings.h"

using namespace std;

Min::Min() {
	strVal = L"XX";
}

void Min::clear() {
	strVal = L"XX";
}

Min::Min(int val) {
	strVal = L"XX";
	if ((val >= 0) && (val <= 59)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Min::Min(const wstring &min) {
	strVal = L"XX";
	int val = Strings::parseInt(min);
	if ((val >= 0) && (val <= 59)) {
		intVal = val;
		strVal = Strings::valueOf(val);
		if (strVal.length() < 2) strVal = L"0" + strVal;
		underspecified = false;
	}
}

Min* Min::clone() const {
	Min* temp = _new Min();
	temp->intVal = intVal;
	temp->strVal = strVal;
	temp->underspecified = underspecified;
	return temp;
}
