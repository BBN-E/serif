// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Assert.h"

// Version of SerifCallAssertMsg() that lets you specify the calling location.
void CallAssertFunction(bool condition, const char* location,
						const char* message)
{
	CallAssertFunctionInline(condition, location, message);
}
