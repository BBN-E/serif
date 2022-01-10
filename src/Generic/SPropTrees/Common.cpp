// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/SPropTrees/Common.h"

bool Common::DEBUG_MODE = false;
int Common::verbose = 0;

std::wstring operator+(const std::wstring& s, long l) {
	std::wostringstream wos;
	wos << s << l;
	return wos.str();
}

std::wstring Common::stripTrailingWSymbols(std::wstring& orig, const wchar_t* syms) {
	int last=(int)orig.length()-1;
	while ( last >= 0 && wcschr(syms,orig[last]) != NULL )
		last--;

	if ( last < (int)orig.length() && orig.length() )
		orig.erase(last+1,orig.npos);

	return orig;
}

std::wstring Common::stripLeadingWSymbols(std::wstring& orig, const wchar_t* syms) {
	size_t pos=orig.find_first_not_of(syms);
	if ( pos != orig.npos ) orig = orig.substr(pos);
	else orig = L"";
	return orig;
}
