// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___COMMON_H___
#define ___COMMON_H___

#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>

namespace Common {
	extern bool DEBUG_MODE;
	extern int verbose;
	std::wstring stripTrailingWSymbols(std::wstring&,const wchar_t* = L" \t");
	std::wstring stripLeadingWSymbols(std::wstring&, const wchar_t* = L" \t");
}

std::wstring operator+(const std::wstring&, long);


#endif
