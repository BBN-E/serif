// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STRING_TRANSLITERATOR_H
#define STRING_TRANSLITERATOR_H

#include <boost/shared_ptr.hpp>

#include <wchar.h>

// This is a Generic/specific interface to a function which must
// produce some ASCII representation of a 16-bit string in the
// domain language. This is for making debugging output more
// readable.

class StringTransliterator {
public:
	/** Create and return a new StringTransliterator. */
	static void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) { _factory()->transliterateToEnglish(result, str, max_result_len); }
	/** Hook for registering new StringTransliterator factories */
	struct Factory { 
		virtual ~Factory() {}
		virtual void transliterateToEnglish(char *result, const wchar_t *str, int max_result_len) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~StringTransliterator() {}
	//// This populates result with up to max_result_len characters.
	//static void transliterateToEnglish(char *result,
	//								   const wchar_t *str,
	//								   int max_result_len);
private:
	static boost::shared_ptr<Factory> &_factory();
};

// TB: xx_StringTransliterator.cpp is only #defined for UNSPEC_LANGUAGE
// if you want to use it for a new language you will need to change the #define
// there as well.
//#if defined(ENGLISH_LANGUAGE)
//	#include "English/common/en_StringTransliterator.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/common/ch_StringTransliterator.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/common/ar_StringTransliterator.h"
//#elif defined(HINDI_LANGUAGE)
//	#includee "Hindi/common/hi_StringTransliterator.h"
//#elif defined(BENGALI_LANGUAGE)
//	#includee "Bengali/common/be_StringTransliterator.h"
//#elif defined(THAI_LANGUAGE)
//	#includee "Thai/common/th_StringTransliterator.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/common/kr_StringTransliterator.h"
//#elif defined(URDU_LANGUAGE)
//	#includee "Urdu/common/ur_StringTransliterator.h"
//#elif defined(UNSPEC_LANGUAGE)
//	#includee "Generic/common/xx_StringTransliterator.h"
//#elif defined(FARSI_LANGUAGE)
//	#includee "Generic/common/xx_StringTransliterator.h"
//
//#else
//	#error "common/StringTransliterator.h does not know your language!"
//#endif


#endif
