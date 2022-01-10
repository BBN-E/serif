// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WS_TOKENIZER_H
#define WS_TOKENIZER_H

#include <string>
#include <vector>

class WSTokenizer {
public:
	static void tokenize(std::wstring str, std::vector<std::wstring>& result);
};

#endif
