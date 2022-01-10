// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_BWNORMALIZER_H
#define AR_BWNORMALIZER_H

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/Symbol.h"
class BWNormalizer {

private:
	static char _message[1000];

public:
	static void normalize(const wchar_t* input, wchar_t* output);
	static Symbol normalize(Symbol input) throw (UnrecoverableException);
	static void normalize(const wchar_t* vinput,const wchar_t* nvinput, wchar_t* output);

	static Symbol normalize(Symbol vowel_input, Symbol nv_input) throw (UnrecoverableException);



};
#endif
