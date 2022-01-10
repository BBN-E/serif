// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_STRINGS_H
#define EN_STRINGS_H

#include <string>
#include <iostream>
#include <sstream>

#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"

#include "Generic/linuxPort/serif_port.h"

using namespace std;

class Strings {
public:
	static wstring valueOf(int i) {
		wchar_t buffer[15];
		if (i > 99999999) {			
			SessionLogger::warn("integer_conversion") << "Strings::valueOf(): tried to convert integer larger than 99999999";
#if defined(_WIN32)
			_itow(0, buffer, 10);
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", 0);
#endif
		} else {
#if defined(_WIN32)
			_itow(i, buffer, 10);
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", i);
#endif
		}
		wstring returnValue = buffer;
		return returnValue;
	}

	static wstring valueOf(float f) {
		wstringstream wss;
		wss << f;
		const wstring ws( wss.str() );
		return ws;
	}

	static int parseInt(wstring s) {
		// if s contains non digits, non spaces, then it should not parse
		// also it shouldn't parse if there is only spaces
		bool found_digit = false;
		for (int i = 0; i < (int)s.length(); i++) {
			if (s.at(i) != L' ' && !iswdigit(s.at(i))) 
				return -1;
			if (iswdigit(s.at(i))) 
				found_digit = true;
		}

		if (!found_digit) return -1;
		return _wtoi(s.c_str());
	}

	static float parseFloat(wstring s) {
		int num_periods = 0;
		bool found_digit = false;
		for (int i = 0; i < (int)s.length(); i++) {
			if (s.at(i) != L' ' && !iswdigit(s.at(i)) && s.at(i) != '.') 
				return -1;
			if (iswdigit(s.at(i))) 
				found_digit = true;
			if (s.at(i) != '.')
				num_periods++;
		}
		if (!found_digit) return -1;
		if (num_periods > 1) return -1;

		// convert to chars because _wtof is not ANSI compatible (boost::lexical_cast<double> would also work)
		char narrow_str[101];
		wcstombs(narrow_str, s.c_str(), 100);
		narrow_str[100] = '\0'; // make sure it's null terminated (not guaranteed by wcstombs) 
		return (float)atof(narrow_str);
	}

	static bool startsWith(const wstring &longStr, const wstring &shortStr) {
		return (!longStr.substr(0, shortStr.length()).compare(shortStr));	
	}

	static bool endsWith(const wstring &longStr, const wstring &shortStr) {
		return (!longStr.substr(longStr.length() - shortStr.length()).compare(shortStr));
	}
};

#endif
