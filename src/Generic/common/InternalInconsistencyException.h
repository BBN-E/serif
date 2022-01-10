// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INTERNAL_INCONSISTENCY_EXCEPTION_H
#define INTERNAL_INCONSISTENCY_EXCEPTION_H

#include "Generic/common/UnrecoverableException.h"

#include <iostream>
#include "string.h"

#include "Generic/linuxPort/serif_port.h"

/** This is a special case of UnrecoverableException. It is thrown
  * whenever a routine can't continue because of some inconsistent
  * state which must have been caused by some bug. */

class InternalInconsistencyException : public UnrecoverableException {
public:
	InternalInconsistencyException(const char *source, const char *message);
	InternalInconsistencyException(const char *source, const std::wstringstream& wss);

	// Instead of having separate classes like we used to, let's put
	// functions down here for creating specialized message strings.
	/** This one is for creating index out-of-bounds exceptions */
	static InternalInconsistencyException arrayIndexException(const char *source, int bound, int index);
	static InternalInconsistencyException arrayIndexException(const char *source, size_t bound, int index) {
		return arrayIndexException(source, static_cast<int>(bound), index);
	}
	static InternalInconsistencyException arrayIndexException(const char *source, int bound, size_t index) {
		return arrayIndexException(source, bound, static_cast<int>(index));
	}
	static InternalInconsistencyException arrayIndexException(const char *source, size_t bound, size_t index) {
		return arrayIndexException(source, static_cast<int>(bound), static_cast<int>(index));
	}
};


#endif
