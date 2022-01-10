// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

//#if defined(UNSPEC_LANGUAGE) || defined(FARSI_LANGUAGE)
#include "Generic/common/leak_detection.h"

#include "Generic/common/xx_StringTransliterator.h"

#include <string>
#include "Generic/linuxPort/serif_port.h"

/** 
  * This populates <code>result</code> with up to 
  * <code>max_result_len</code> characters.
  *
  * @param result the resulting ASCII representation
  * @param str the 16-bit string to translate
  * @param max_result_len the maximum length of <code>result</code>
  */
void GenericStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t *str,
												  int max_result_len) 
{ 

	// THIS IS A COPY OF THE FUNCTION FROM ENGLISH -- FIX ME!!

	if (str == 0) {
		strncpy(result, "(null)", max_result_len);
		return;
	}
	else {
		size_t pos = 0;
		size_t n = wcslen(str);
		for (size_t i = 0; i < n; i++) {
			wchar_t c = str[i];

			char english_c = (char) c;

			if ((english_c == '\t') ||
				(english_c == '\n') ||
				((english_c >= 0x20) && ((wchar_t)english_c == c)))
			{
				result[pos++] = english_c;
				if (pos == (size_t)max_result_len - 1)
					break;
			}
		}
		result[pos] = '\0';
	}
}
//#endif
