// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/BWMorph/BWFunctions.h"

bool BWFunctions::isDiacritic(wchar_t c){
	bool ret_val = ((c == L'\x64E') ||
				(c == L'\x64F') ||
				(c == L'\x650') ||
				(c == L'\x651') ||
				(c == L'\x652'));
	return ret_val;
}
bool BWFunctions::isPartOfDictKey(wchar_t c){
	bool ret_val = (isDiacritic(c) || 
				(c == L'\x670') ||	//these don't have win encoding
				(c == L'\x671') ||
				(c == L'\x6A4') ||
				(c == L'\x640'));		//removed separately from diacritics
	return ret_val;
}
	
