// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnsupportedOperationException.h"

UnsupportedOperationException::UnsupportedOperationException(const char *source, const char *message) {
	strcpy_s(_source, source);
	strcpy_s(_message, "UnsupportedOperationException: ");
	strncat_s(_message, message, MAX_STR_LEN - 31);
}

UnsupportedOperationException::UnsupportedOperationException(const char *source, const std::wstringstream& wss) {
	strcpy_s(_source, source);
	strcpy_s(_message, "UnsupportedOperationException: ");
	std::wstring ws = wss.str();
	std::string s(ws.begin(), ws.end());
	strncat_s(_message, s.c_str(), MAX_STR_LEN - 31);
}
