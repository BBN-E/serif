// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"

InternalInconsistencyException::InternalInconsistencyException(const char *source, const char *message) {
	strcpy_s(_source, source);
	std::stringstream ss;
	ss << "Internal Serif inconsistency in " << source << ": " << message;
	sprintf(_message, "%s", ss.str().substr(0, MAX_STR_LEN).c_str());
}

InternalInconsistencyException::InternalInconsistencyException(const char *source, const std::wstringstream& wss) {
	strcpy_s(_source, source);
	strcpy_s(_message, "Internal Serif inconsistency: ");
	std::wstring ws = wss.str();
	std::string s(ws.begin(), ws.end());
	strncat_s(_message, s.c_str(), MAX_STR_LEN-30);
}


InternalInconsistencyException InternalInconsistencyException::arrayIndexException(const char *source, int bound, int index) {
	char s[100];
	sprintf_s(s, "Array index out of bounds (bound = %d, index = %d)",
			  bound, index);
	return InternalInconsistencyException(source, s);
}
