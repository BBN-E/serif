// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OUTPUT_STREAM_H
#define OUTPUT_STREAM_H

#include <string>


// abstract interface for our UTF8OutputStream and OStringStream

class OutputStream {
public:
	virtual ~OutputStream() {}

	virtual void flush() = 0;

	virtual const std::wstring &getFileName() const = 0;

	virtual OutputStream& put(wchar_t ch) = 0;
	virtual OutputStream& write(const wchar_t* str, size_t size) = 0;

	virtual OutputStream &operator<<(const std::wstring& str) = 0;
	virtual OutputStream &operator<<(const wchar_t *str) = 0;
	virtual OutputStream &operator<<(const char *str) = 0;
	virtual OutputStream &operator<<(int i) = 0;
	virtual OutputStream &operator<<(double d) = 0;

	virtual void close() = 0;
};

#endif
