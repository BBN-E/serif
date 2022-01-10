// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"


#include "English/timex/Year.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Strings.h"
#include "Generic/common/Symbol.h"
#include <ctype.h>
#include <math.h>
#include <string>
#include <iostream>

using namespace std;

Year::Year() {
	strVal = L"XXXX";
}

void Year::clear() {
	strVal = L"XXXX";
}

Year::Year(int val) {
	strVal = L"XXXX";
	if ((val >= 0) && (val < 10)) {
		strVal = L"XX0" + Strings::valueOf(val); 
	} else if ((val >= 10) && (val < 100)) {
		strVal = L"XX" + Strings::valueOf(val);
	} else if ((val >= 100) && (val <= 9999)) {
		intVal = val;
		strVal = Strings::valueOf(intVal);
		while (strVal.length() < 4) {
			strVal = L"0" + strVal;
		}
		underspecified = false;
	}
}

Year::Year(const wstring &year) {
	strVal = L"XXXX";
	int val = Strings::parseInt(year);
	if ((val >= 0) && (val < 10)) {
		strVal = L"XX0" + Strings::valueOf(val);
	} else if ((val >= 10) && (val < 100)) {
		strVal = L"XX" + Strings::valueOf(val);
	} else if ((val >= 100) && (val <= 9999)) {
		intVal = val;
		strVal = Strings::valueOf(intVal);
		while (strVal.length() < 4) {
			strVal = L"0" + strVal;
		}
		underspecified = false;
	} else {
		if (year.length() > 4) 
			return;
		for (size_t i = 0; i < year.length(); i++)	
			if (!iswdigit(year[i]) && year[i] != 'X')
				return;
		strVal = year;
		while (strVal.length() < 4) {
			strVal = L"X" + strVal;
		}
	}
}

Year* Year::clone() const {
	Year* temp = _new Year();
	temp->intVal = intVal;
	temp->strVal = strVal;
	temp->underspecified = underspecified;
	return temp;
}

Year* Year::sub(int val) const {
	Year* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal -= val;
		size_t fill = temp->strVal.length();
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
	} else { 
		// try to subtract from underspecified form
		wstring yearBegin = L"";
		int i = 0;
		while (iswdigit(temp->strVal[i])) {
			yearBegin += temp->strVal[i];
			i++;
		}
		if ( i > 0) {
			val = (int)(val / (int) pow((double)10,(4 - i)));
			val = Strings::parseInt(temp->strVal.substr(0, i)) - val;
			temp->strVal = Strings::valueOf(val);
			for (int j = i; j < 4; j++) {
				temp->strVal += L"X";
			}
		}	
	}
	return temp;
}

Year* Year::add(int val) const {
	Year* temp = this->clone();
	if (!temp->underspecified) {
		temp->intVal += val;
		size_t fill = temp->strVal.length();
		temp->strVal = Strings::valueOf(temp->intVal);
		while (temp->strVal.length() < fill) {
			temp->strVal = L"0" + temp->strVal;
		}
	} else { 
		// try to subtract from underspecified form
		wstring yearBegin = L"";
		int i = 0;
		while (iswdigit(temp->strVal[i])) {
			yearBegin += temp->strVal[i];
			i++;
		}
		if ( i > 0) {
			val = (int)(val / (int) pow((double)10, (4 - i)));
			val = Strings::parseInt(temp->strVal.substr(0, i)) + val;
			temp->strVal = Strings::valueOf(val);
			for (int j = i; j < 4; j++) {
				temp->strVal += L"X";
			}
		}	
	}
	return temp;
}

bool Year::leapYearP(int yr) {
	if (yr < 0) 
		return false;
	if (yr % 400 == 0) 
		return true; 
	if (yr % 100 == 0) 
		return false; 
	if (yr % 4 == 0) 
		return true;  
	return false;
}

wstring Year::canonicalizeYear(int n) {
	if (n < 0) return L"";
	if (n < 10) return Strings::valueOf(2000 + n);
	if (n < 100) return Strings::valueOf(1900 + n);
	if (n < 1000) return L"0" + Strings::valueOf(n);
	return Strings::valueOf(n);
}	
