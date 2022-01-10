// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ISTRINGSTREAM_H
#define ISTRINGSTREAM_H

#include <sstream>

typedef std::basic_istringstream<wchar_t> IStringStream;
/*

#include <wchar.h>
#include "Generic/common/InputStream.h"

// A string input-stream class.
// Bare bones for now, can be supplemented with additional stream capabilities if there's
//  a need.

class IStringStream : public InputStream {
public:
	//copies the buffer, and then owns the copy (i.e. deletes it upon stream's destruction)
	IStringStream(const wchar_t *buf) {
		_buf = new wchar_t[wcslen(buf)+1];
		wcscpy(_buf, buf);
		_bufLen = wcslen(buf);
		_pos = 0;
	}
	
	virtual ~IStringStream() {
		delete _buf;
	}

	virtual const char* getFileName() const { return NULL; }

	
	// retrieve a single wide character
	virtual wchar_t get() {
		if(!eof())
			return _buf[_pos++];
		else return 0;
	}

	// "eof" figuratively means you've reached the end of the stream.
	bool eof() {
		return (_pos >= _bufLen);
	}

private:
	wchar_t *_buf;
	size_t _pos, _bufLen;
};
*/

#endif
