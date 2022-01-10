#ifndef AR_STRING_TRANSLITERATOR_H
#define AR_STRING_TRANSLITERATOR_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// See common/ArabicStringTransliterator.h

#include "Generic/common/StringTransliterator.h"


class ArabicStringTransliterator : public StringTransliterator {
private:
	friend class ArabicStringTransliteratorFactory;

public:
	static void transliterateToEnglish(char *result,
									   const wchar_t *str,
									   int max_result_len);
//copied from ArabicSymbol- can't use symbol
private:
	static wchar_t _mapping[256];
	static wchar_t *getMapping();

};

class ArabicStringTransliteratorFactory: public StringTransliterator::Factory {
	virtual void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) {  return ArabicStringTransliterator::transliterateToEnglish(result, str, max_result_len); }
};



#endif
