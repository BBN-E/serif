// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNSUPPORTED_OPERATION_EXCEPTION_H
#define UNSUPPORTED_OPERATION_EXCEPTION_H

#include "Generic/common/UnrecoverableException.h"

/** This exception is to be throw whenever an operation is not (or no more) supported.
*/

class UnsupportedOperationException : public UnrecoverableException {
public:
	UnsupportedOperationException(const char *source, const char *message);

	UnsupportedOperationException(const char *source, const std::wstringstream& wss);
};

#endif

