// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_BUCKWALTERFUNCTIONS_H
#define AR_BUCKWALTERFUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/LexicalEntry.h"

class BuckwalterFunctions{
public:
	static int pullLexEntriesUp(LexicalEntry** result, int size, LexicalEntry* initialEntry) throw (UnrecoverableException);
	static Symbol UNKNOWN;
	static Symbol NULL_SYMBOL;
	static Symbol ANALYZED;
	static Symbol RETOKENIZED;
	static Symbol NON_ARABIC;
	bool static _equivalentChars(wchar_t a, wchar_t b);

};
#endif
