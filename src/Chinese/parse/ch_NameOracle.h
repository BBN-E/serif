// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_NAME_ORACLE_H
#define ch_NAME_ORACLE_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"

class ChineseNameOracle {
public:
	ChineseNameOracle() {}
    bool isName(ChartEntry *entry, Symbol chain) const;
};

#endif
