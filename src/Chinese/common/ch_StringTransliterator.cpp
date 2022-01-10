// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/common/ch_StringTransliterator.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/linuxPort/serif_port.h"


void ChineseStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t *str,
												  int max_result_len)
{
	int pos = 0;
	int n = static_cast<int>(wcslen(str));
	

	for (int i = 0; i < n; i++) {
		wchar_t c = str[i];
		if (c == 0x0000)
			break;
		if (c <= 0xff) {
			if (pos >= max_result_len - 1) {
				SessionLogger::warn("transliteration") << "ChineseStringTransliterator::transliterateToEnglish()," 
						   << " Transliteration truncated for length\n";
				break;
			}
			result[pos++] = (char)c;
		}
		else {
			if (pos >= max_result_len - 6) {
				SessionLogger::warn("transliteration") << "ChineseStringTransliterator::transliterateToEnglish(),"
					   << " Transliteration truncated for length\n";
				break;
			}
			char hex_str[5];
			_itoa(c, hex_str, 16);
			result[pos++] = '\\';
			result[pos++] = 'x';
			size_t len = strlen(hex_str);
			// pad with zeros
			for (size_t j = len; j < 4; j++) 
				result[pos++] = '0';
			for (size_t i = 0; i < len; i++) 		
				result[pos++] = hex_str[i];
		}
	}
	result[pos] = '\0';
}

void ChineseStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t ch,
												  int max_result_len)
{
	wchar_t ch_str[2];

	ch_str[0] = ch;
	ch_str[1] = 0x0000;
	
	transliterateToEnglish(result, ch_str, max_result_len);
}

