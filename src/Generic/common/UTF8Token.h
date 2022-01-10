// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UTF8_TOKEN_H
#define UTF8_TOKEN_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Symbol.h"

#include <cstddef>

#define UTF8_TOKEN_BUFFER_SIZE 256


class UTF8Token {
private:
	friend std::wistream& operator>>(std::wistream& stream, UTF8Token& token)
		throw(UnexpectedInputException);
	static const size_t buffer_size;
	wchar_t buffer[UTF8_TOKEN_BUFFER_SIZE];
	Symbol symbol;
	bool symbol_is_valid;

public:
	UTF8Token(): symbol_is_valid(false) {}
	~UTF8Token() {}
	const wchar_t* chars() {return buffer;}

	// The reference returned by this method is invalidated when a new
	// token is read (using operator>>).
	const Symbol& symValue() {
		if (!symbol_is_valid) {
			symbol_is_valid = true;
			symbol = Symbol(buffer);
		}
		return symbol;
	}
};

#endif

