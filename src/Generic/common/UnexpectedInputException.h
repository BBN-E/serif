// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNEXPECTED_INPUT_EXCEPTION_H
#define UNEXPECTED_INPUT_EXCEPTION_H

#include "Generic/common/UnrecoverableException.h"

/** This is a special case of UnrecoverableException. It is thrown
  * whenever some external input (parameters, a model file, the input
  * data, etc.) doesn't make sense. */

class UnexpectedInputException : public UnrecoverableException {
public:
	UnexpectedInputException(const char *source, const char *message);

	// a convenience function that concatenates the last 2 arguments
	UnexpectedInputException(const char *source, const char *messageStart, const char *messageEnd);

	// Convenience function that takes a std::wstringstream.
	// Obviously, if you actually have non-ascii characters you are going to lose them.
	UnexpectedInputException(const char *source, const std::wstringstream& wss);
};

#endif

