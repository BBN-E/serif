// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ARABIC_SYMBOL_H
#define AR_ARABIC_SYMBOL_H

#include "Generic/common/Symbol.h"

#include <wchar.h>


class ArabicSymbol : public Symbol {
private:
	static const int MAX_SYMBOL_LEN = 1000;

public:
	ArabicSymbol() : Symbol() {}

	ArabicSymbol(const wchar_t *s);
	static Symbol BWSymbol(const wchar_t *s);
	static wchar_t ArabicChar(char c);
private:

	static wchar_t _mapping[256];
	static wchar_t *getMapping();
};

#endif
