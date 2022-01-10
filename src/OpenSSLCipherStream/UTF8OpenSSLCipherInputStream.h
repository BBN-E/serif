// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UTF8_OPEN_SSL_CIPER_INPUT_STREAM_H
#define UTF8_OPEN_SSL_CIPER_INPUT_STREAM_H

#include "Generic/common/UTF8InputStream.h"
#include <sstream>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class UTF8OpenSSLCipherInputStream: public UTF8InputStream {
public:
	UTF8OpenSSLCipherInputStream (const char *key, const char *file);
	virtual ~UTF8OpenSSLCipherInputStream ();

	virtual void open (const char *file);
	virtual void open (const wchar_t *file);
	virtual void close();
	bool is_open() { return _is_open || UTF8InputStream::is_open(); }
	std::wstreambuf* rdbuf() { return _decrypted_stream.rdbuf(); }
  
private:
	std::wistringstream _decrypted_stream;
	std::string _key;
	bool _is_open;
};

#endif
