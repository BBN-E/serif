// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

/* */
#include <iostream>

typedef std::basic_istream<wchar_t> InputStream;
/* */

/* /
// An abstract input-stream class.
// Bare bones for now, can be supplemented with additional stream capabilities if there's
//  a need.

class InputStream {
public:
	virtual ~InputStream() {}
	// retrieve a single wide character
	virtual wchar_t get() = 0;
	virtual const char* getFileName() const = 0;

};
*/

#endif
