// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CHAIN_FINDER_H
#define CHAIN_FINDER_H

#include <cstddef>
#include <string>
#include "Generic/parse/ParseNode.h"

class ChainFinder {
public:
	static ParseNode* find(ParseNode* parse, std::wstring& str);
};

#endif


