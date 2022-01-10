// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/KoreanStringTransliterator.h"
#include "common/SessionLogger.h"


void KoreanStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t *str,
												  int max_result_len)
{
	SessionLogger &logger = *SessionLogger::logger;
	int pos = 0;
	int n = static_cast<int>(wcslen(str));
	

	for (int i = 0; i < n; i++) {
		wchar_t c = str[i];
		if (c == 0x0000)
			break;
		if (c <= 0xff) {
			if (pos >= max_result_len - 1) {
				if (&logger != 0) {
					logger.beginWarning();
					logger << "KoreanStringTransliterator::transliterateToEnglish()," 
						   << " Transliteration truncated for length\n";
				}
				break;
			}
			result[pos++] = (char)c;
		}
		else {
			if (pos >= max_result_len - 6) {
				logger.beginWarning();
				logger << "KoreanStringTransliterator::transliterateToEnglish(),"
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

void KoreanStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t ch,
												  int max_result_len)
{
	wchar_t kr_str[2];

	kr_str[0] = ch;
	kr_str[1] = 0x0000;
	
	transliterateToEnglish(result, kr_str, max_result_len);
}

