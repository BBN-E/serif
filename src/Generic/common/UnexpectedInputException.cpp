// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"

UnexpectedInputException::UnexpectedInputException(const char *source, const char *message) {
	strcpy_s(_source, source);
	strcpy_s(_message, "Unexpected Input Exception: ");
	strncat_s(_message, message, MAX_STR_LEN - 30);
}

UnexpectedInputException::UnexpectedInputException(const char *source, const char *messageStart, const char *messageEnd) {
	strcpy_s(_source, source);
	strcpy_s(_message, "Unexpected Input Exception: ");
	strncat_s(_message, messageStart, MAX_STR_LEN - 30);
	size_t msgLen = strlen(messageStart);
	strncat_s(_message, messageEnd, MAX_STR_LEN - msgLen - 30);
}

UnexpectedInputException::UnexpectedInputException(const char *source, const std::wstringstream& wss) {
	strcpy_s(_source, source);
	strcpy_s(_message, "Unexpected Input Exception: ");
	std::wstring ws = wss.str();
	std::string s(ws.begin(), ws.end());
	strncat_s(_message, s.c_str(), MAX_STR_LEN - 30);
}
