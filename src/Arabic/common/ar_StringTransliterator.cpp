// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/common/ar_StringTransliterator.h"

#include "string.h"
#include <stdio.h>
wchar_t ArabicStringTransliterator::_mapping[256];

wchar_t *ArabicStringTransliterator::getMapping() {
	static bool _mapping_initialized = false;

	if (!_mapping_initialized) {
		for (int i = 0; i < 256; i++)
			_mapping[i] = static_cast<wchar_t>(i);

		_mapping['\''] = L'\x0621';
		_mapping['|']  = L'\x0622';
		_mapping['>']  = L'\x0623';
		_mapping['&']  = L'\x0624';
		_mapping['<']  = L'\x0625';
		_mapping['}']  = L'\x0626';
		_mapping['A']  = L'\x0627';
		_mapping['b']  = L'\x0628';
		_mapping['p']  = L'\x0629';
		_mapping['t']  = L'\x062A';
		_mapping['v']  = L'\x062B';
		_mapping['j']  = L'\x062C';
		_mapping['H']  = L'\x062D';
		_mapping['x']  = L'\x062E';
		_mapping['d']  = L'\x062F';
		_mapping['*']  = L'\x0630';
		_mapping['r']  = L'\x0631';
		_mapping['z']  = L'\x0632';
		_mapping['s']  = L'\x0633';
		_mapping['$']  = L'\x0634';
		_mapping['S']  = L'\x0635';
		_mapping['D']  = L'\x0636';
		_mapping['T']  = L'\x0637';
		_mapping['Z']  = L'\x0638';
		_mapping['E']  = L'\x0639';
		_mapping['g']  = L'\x063A';
		_mapping['_']  = L'\x0640';
		_mapping['f']  = L'\x0641';
		_mapping['q']  = L'\x0642';
		_mapping['k']  = L'\x0643';
		_mapping['l']  = L'\x0644';
		_mapping['m']  = L'\x0645';
		_mapping['n']  = L'\x0646';
		_mapping['h']  = L'\x0647';
		_mapping['w']  = L'\x0648';
		_mapping['Y']  = L'\x0649';
		_mapping['y']  = L'\x064A';
		_mapping['F']  = L'\x064B';
		_mapping['N']  = L'\x064C';
		_mapping['K']  = L'\x064D';
		_mapping['a']  = L'\x064E';
		_mapping['u']  = L'\x064F';
		_mapping['i']  = L'\x0650';
		_mapping['~']  = L'\x0651';
		_mapping['o']  = L'\x0652';
		_mapping['`']  = L'\x0670';
		_mapping['{']  = L'\x0671';
		_mapping['P']  = L'\x067E';
		_mapping['J']  = L'\x0686';
		_mapping['V']  = L'\x06A4';
		_mapping['G']  = L'\x06AF';

		_mapping_initialized = true;
	}

	return _mapping;
}

void ArabicStringTransliterator::transliterateToEnglish(char *result,
												  const wchar_t *str,
												  int max_result_len)
{
	// This is just a temporary job so this thing compiles
	wchar_t *mapping = getMapping();

	if (str == 0) {
		strncpy(result, "(null)", max_result_len);
		return;
	}
	else {
		int pos = 0;
		size_t n = wcslen(str);
		for (size_t i = 0; i < n && pos < max_result_len - 5; i++) {
			wchar_t arabicChar =  str[i];
			char english_c = '@';
			if((int)arabicChar <256){
				english_c = (char)arabicChar;
			}
			else{
				for(int j = 0; j< 256; j++){
					if(mapping[j] == str[i]){
						english_c = static_cast<char>(j);
						break;
					}
				}
			}
			result[pos++] = english_c;
		}
		result[pos] = '\0';
	}
}
