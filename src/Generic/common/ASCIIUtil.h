// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ASCII_UTIL_H
#define ASCII_UTIL_H

#include <string>

class ASCIIUtil {
public:
	static bool containsAlpha(const std::wstring &str);
	static bool containsNonWhitespace(const std::wstring &str);
	static bool containsDigits(const std::wstring &str);
	static bool containsNonDigits(const std::wstring &str);
};

#endif
