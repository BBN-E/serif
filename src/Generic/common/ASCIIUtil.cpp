// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ASCIIUtil.h"
#include "Generic/linuxPort/serif_port.h"
#include <iostream> // for iswspace on linux

bool ASCIIUtil::containsAlpha(const std::wstring &str) {
	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		int ci = (int) c;
		// iswalpha() doesn't work here for some reason
		if (ci >= L'a' && ci <= L'z') return true;
		if (ci >= L'A' && ci <= L'Z') return true;
	}
	return false;
}

bool ASCIIUtil::containsNonWhitespace(const std::wstring &str) {
	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		if (!iswspace(c))
			return true;
	}
	return false;
}

bool ASCIIUtil::containsDigits(const std::wstring &str) {
	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		int ci = (int) c;
		// Using iswdigit() causes code points such as 3670 (Thai digit "six") to be treated as digits, which is not desired.
		if (ci >= L'0' && ci <= L'9')
			return true;
	}
	return false;
}

bool ASCIIUtil::containsNonDigits(const std::wstring &str) {
	for (size_t i = 0; i < str.length(); i++) {
		wchar_t c = str.at(i);
		int ci = (int) c;
		// Using iswdigit() causes code points such as 3670 (Thai digit "six") to be treated as digits, which is not desired.
		if (ci < L'0' || ci > L'9')
			return true;
	}
	return false;
}
