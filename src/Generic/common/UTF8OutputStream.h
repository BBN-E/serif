// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UTF8_OUTPUT_STREAM_H
#define UTF8_OUTPUT_STREAM_H

#include "Generic/common/OutputStream.h"

#include <fstream>
#include <wchar.h>
#include <string>
#include <boost/shared_ptr.hpp>


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

// write unicode wide chars (and Symbols?) as utf8 files
// sort of like an ostream, but not supportive of the full set of
// methods

// see also UTF8InputStream

class SERIF_EXPORTED UTF8OutputStream : public OutputStream {
protected:
	std::basic_ofstream</*unsigned*/ char> _stream;
	std::wstring _file;
public:
	struct Factory {
		virtual ~Factory() {}
		virtual UTF8OutputStream *build(const char* filename, 
										bool encrypted=false);
	};
	static void setFactory(boost::shared_ptr<Factory> factory);
	static UTF8OutputStream *build(const char* filename, bool encrypted);
	// These build methods are provided for convenience
	static UTF8OutputStream *build(std::string filename, bool encrypted);
	static UTF8OutputStream *build(std::wstring filename, bool encrypted);
	static UTF8OutputStream *build(const wchar_t* filename, bool encrypted);

	UTF8OutputStream(const std::string file);
	UTF8OutputStream(const std::wstring file);
	UTF8OutputStream(const char* file);
	UTF8OutputStream(const wchar_t* file);
	UTF8OutputStream();
	virtual ~UTF8OutputStream();

	virtual void open(const char* file, bool append = false);
	virtual void open(const wchar_t* file, bool append = false);
	virtual void close();

	/** Use this to determine if open() worked */
	// BUT: we throw an exception if open failed!!
	virtual bool fail() { return _stream.fail(); }

	// place a single wide character as a utf8 sequence of bytes
	// returns *this
	virtual UTF8OutputStream& put(wchar_t ch);
	// place a sequence of wide chars
	// returns *this
	virtual UTF8OutputStream& write(const wchar_t* str, size_t size);

	virtual void flush() { _stream.flush(); }

	virtual const std::wstring &getFileName() const; 

	// other convenient writing utilities
	UTF8OutputStream &operator<<(const std::wstring& str) {
		return write(str.c_str(), str.length());
	}
	UTF8OutputStream &operator<<(const wchar_t* str) {
		return write(str, wcslen(str));
	}
	UTF8OutputStream& operator<< (std::string str);
	UTF8OutputStream& operator<< (const char* str);
	UTF8OutputStream& operator<< (int i);
	UTF8OutputStream& operator<< (double i);
	UTF8OutputStream& operator<< (char c);
	// note: size_t will be either 'unsigned int' or 'unsigned long
	// int', depending on whether we're compiling on 32 bit or 64 bit.
	UTF8OutputStream& operator<< (unsigned int i);
	UTF8OutputStream& operator<< (unsigned long int i);
	UTF8OutputStream& operator<< (unsigned long long int i);
	
private:
	static boost::shared_ptr<Factory> &_factory();
};

#endif
