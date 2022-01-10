// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "WSTokenizer.h"
#include <cwctype>

using namespace std;

void WSTokenizer::tokenize(wstring str, vector<wstring>& result) {
	size_t index = 0;
	size_t len = str.length();
	while (index < len) {
		while ((index < len) && iswspace(str.at(index))) {
			++index;
		}
		if (index < len) {
			wstring t = L"";
			while ((index < len) && !iswspace(str.at(index))) {
				t += str.at(index++);
			}
			result.push_back(t);
		}
	}
}
