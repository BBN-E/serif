// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_NAME_ORACLE_H
#define ar_NAME_ORACLE_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"

class ArabicNameOracle {
public:
	ArabicNameOracle() {}
    bool isName(ChartEntry *entry, Symbol chain) const;
};

#endif
