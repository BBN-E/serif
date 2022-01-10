// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CAPITAL_A_ASSERT_H
#define CAPITAL_A_ASSERT_H

#include <iostream>
#include <sstream>
#include "Generic/common/InternalInconsistencyException.h"

// Assertion facility.  Summary:
// SerifAssert(cond)					NDEBUG => no-op, else: cond || throw exception
// SerifAssertMsg(cond, msg)			same, add a custom message to the exception
// SerifCallAssert, SerifCallAssertMsg	same, but run even with NDEBUG

// SerifAssert() and SerifAssertMsg() compile to no-ops when NDEBUG is #defined.  In
// Windows, NDEBUG disables assert(); in Serif, typically, NDEBUG is #defined
// for Release configurations only.  If you want to be less selective, use
// SerifCallAssert() directly.  If you want to be more selective, wrap your calls
// in something else, or use the condition arguments.
#ifdef NDEBUG
#define SerifAssert(condition)                             do{} while (false)
#define SerifAssertMsg(condition, messageStreamExpression) do{} while (false)
#else

// If 'condition' is false, throw IntenalInconsistencyException 
// with file:line location and no message.
// Example usage: SerifAssert(arg3.isGood());
#define SerifAssert(condition) \
	SerifCallAssert(condition)

// If 'condition' is false, throw IntenalInconsistencyException with
// file:line location and message given by the stream expression.
// Example usage: SerifAssertMsg(arg3.isGood(), "arg3 " << arg3 << " is bad");
// Caution: InternalInconsistencyException truncates long messages.
#define SerifAssertMsg(condition, messageStreamExpression) \
	SerifCallAssertMsg(condition, messageStreamExpression)

#endif

// Version of SerifAssert() that's available even when NDEBUG is defined.
#define SerifCallAssert(condition) \
do { \
	if (!(condition)) \
		CallAssertFunction(false, AssertHere(__FILE__, __LINE__).c_str()); \
} \
while (false)

// Version of SerifAssertMsg() that's available even when NDEBUG is defined.
#define SerifCallAssertMsg(condition, messageStreamExpression) \
do { \
	if (!(condition)) { \
		std::ostringstream message; \
		message << messageStreamExpression; \
		CallAssertFunction(false, AssertHere(__FILE__, __LINE__).c_str(), \
						   message.str().c_str()); \
	} \
} \
while (false)

// Return file:line as a string.
// You can implement "__FILE__:__LINE__" as a constant expression as below;
// but we don't do it because of MSVC bug Q199057, in which /ZI (Program
// Database with Edit and Continue) breaks the string-ized version of
// __LINE__; and because our SerifAssert() macros don't require aggressive
// evaluation of the location.
// #define _ASSERT_STR1(line) #line
// #define _ASSERT_STR2(line) _ASSERT_STR1(line)
// #define ASSERT_HERE __FILE__ ":" _ASSERT_STR2(__LINE__)
inline std::string AssertHere(const char* file, const unsigned long line)
{
	std::ostringstream here;
	here << file << ":" << line;
	return here.str();
}

// Version of SerifCallAssertMsg() that lets you specify the calling location.
void CallAssertFunction(bool condition, const char* location,
						const char* message = "");
inline void CallAssertFunctionInline(bool condition, const char* location,
									 const char* message = "")
{
	if (! condition) {
		std::ostringstream exceptionMessage;
		exceptionMessage << "Assert failed";
		if (strlen(message) > 0)
			exceptionMessage << ": " << message;
		throw InternalInconsistencyException
			(const_cast<char*>(location),
			 	// the called function says this is supposed to be a function
				// name, but making it a line # doesn't seem to hurt, and is
				// more precise
			 const_cast<char*>(exceptionMessage.str().c_str()));
	}
}

#endif
