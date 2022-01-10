// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_STRING_TRANSLITERATOR
#define XX_STRING_TRANSLITERATOR

#include "Generic/common/StringTransliterator.h"

/** 
  * Utility class to produce some ASCII representation of a 
  * 16-bit string in an unspecified language. This is for making debugging 
  * output more readable.
  */
class GenericStringTransliterator : public StringTransliterator {
private:
	friend class GenericStringTransliteratorFactory;

public:
	/** 
	  * This populates <code>result</code> with up to 
	  * <code>max_result_len</code> characters.
	  *
	  * @param result the resulting ASCII representation
	  * @param str the 16-bit string to translate
	  * @param max_result_len the maximum length of <code>result</code>
	  */
	static void transliterateToEnglish(char *result,
									   const wchar_t *str,
									   int max_result_len); 
};

class GenericStringTransliteratorFactory: public StringTransliterator::Factory {
	virtual void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) {  GenericStringTransliterator::transliterateToEnglish(result, str, max_result_len); }
};


#endif
