// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_NAME_ORACLE_H
#define en_NAME_ORACLE_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"

class EnglishNameOracle {
public:
	EnglishNameOracle() {}
    bool isName(ChartEntry *entry, Symbol chain) const;
};

#endif

