#ifndef CH_STRING_TRANSLITERATOR_H
#define CH_STRING_TRANSLITERATOR_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/StringTransliterator.h"



/** 
  * Utility class to produce some ASCII representation of a 
  * 16-bit string in the Chinese. This is for making debugging 
  * output more readable.
  */
class ChineseStringTransliterator : public StringTransliterator {
private:
	friend class ChineseStringTransliteratorFactory;

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

	/** 
	  * This populates <code>result</code> with up to 
	  * <code>max_result_len</code> characters.
	  *
	  * @param result the resulting ASCII representation
	  * @param ch the 16-bit character to translate
	  * @param max_result_len the maximum length of <code>result</code>
	  */
	static void transliterateToEnglish(char *result,
									   const wchar_t ch,
									   int max_result_len);
};

class ChineseStringTransliteratorFactory: public StringTransliterator::Factory {
	virtual void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) {  ChineseStringTransliterator::transliterateToEnglish(result, str, max_result_len); }
};



#endif
