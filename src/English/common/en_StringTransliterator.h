#ifndef EN_STRING_TRANSLITERATOR_H
#define EN_STRING_TRANSLITERATOR_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/StringTransliterator.h"


// See common/EnglishStringTransliterator.h

class EnglishStringTransliterator : public StringTransliterator {
private:
	friend class EnglishStringTransliteratorFactory;

public:
	static void transliterateToEnglish(char *result,
									   const wchar_t *str,
									   int max_result_len);
};

class EnglishStringTransliteratorFactory: public StringTransliterator::Factory {
	virtual void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) {  EnglishStringTransliterator::transliterateToEnglish(result, str, max_result_len); }
};



#endif
