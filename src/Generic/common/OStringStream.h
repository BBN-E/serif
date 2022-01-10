// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef O_STRING_STREAM_H
#define O_STRING_STREAM_H

#include "Generic/common/OutputStream.h"

#include <string.h>
#include <wchar.h>
#include <string>


class OStringStream : public OutputStream {
public:
	OStringStream(std::wstring &string) : _string(string) {}
	virtual ~OStringStream() {}

	virtual void flush() {}

	virtual const std::wstring &getFileName() const { return _emptyString; }

	virtual OutputStream &put(wchar_t ch) {
		_string += ch;
		return *this;
	}
	virtual OutputStream &write(const wchar_t* str, size_t size) {
		_string += str;
		return *this;
	}

	virtual OutputStream &operator<<(const std::wstring& str) {
		_string += str;
		return *this;
	}
	virtual OutputStream &operator<<(const wchar_t* str) {
		_string += str;
		return *this;
	}
	virtual OutputStream &operator<<(const char* str) {
		size_t len = strlen(str);
		wchar_t *buf = _new wchar_t[len+1];
		for (size_t i = 0; i < len+1; i++)
			buf[i] = static_cast<wchar_t>(str[i]);
		_string += buf;
		delete[] buf;
		return *this;
	}
	virtual OutputStream &operator<<(int i) {
		wchar_t buf[10];
#if defined(_WIN32)
		swprintf_s(buf, L"%d", i);
#else
		swprintf(buf, 10, L"%d", i);
#endif
		_string += buf;
		return *this;
	}

	virtual OutputStream &operator<<(double d) {
		wchar_t buf[10];
#if defined(_WIN32)
		swprintf_s(buf, L"%f", d);
#else
		swprintf(buf, 10, L"%f", d);
#endif
		_string += buf;
		return *this;
	}

	virtual void close() {
		// do nothing (MRK: TODO: is this the most correct implementation?
		//			   maybe it is because we cannot actually "detach" from our internal string alias.
		//			   it might be more correct to actually have this flip an internal "isClosed" boolean, 
		//			   and have all the <<, etc. functions query isClosed to see if they're allowed to 
		//			   write to the string anymore. ??? who cares)
	}

private:
	std::wstring &_string;
	std::wstring _emptyString; // used for filename
};

#endif
