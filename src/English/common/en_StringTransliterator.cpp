// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_StringTransliterator.h"

#include "string.h"


void EnglishStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t *str,
												  int max_result_len)
{
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
