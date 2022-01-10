// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_UTF8XML_TOKEN_H
#define AR_UTF8XML_TOKEN_H

#include "Generic/common/UTF8InputStream.h"
#include <cstddef>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"

#define UTF8_TOKEN_BUFFER_SIZE 256


class UTF8XMLToken {
private:
	static void GetWholeTag(UTF8InputStream& stream, UTF8XMLToken& token, int size); 
	friend UTF8InputStream& operator>>(UTF8InputStream& stream, UTF8XMLToken& token)
		throw(UnexpectedInputException);
	static const size_t buffer_size;
	wchar_t buffer[UTF8_TOKEN_BUFFER_SIZE];
	wchar_t _subst_buffer[UTF8_TOKEN_BUFFER_SIZE*2]; //only for SubstSymbolValue method- TODO:remove this
	
public:
	UTF8XMLToken() {}
	~UTF8XMLToken() {}
	const wchar_t* chars() {return buffer;}
	Symbol symValue() {return Symbol(buffer);}
	Symbol SubstSymValue();

};

#endif

