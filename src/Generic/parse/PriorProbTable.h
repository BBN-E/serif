// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRIOR_PROB_TABLE_H
#define PRIOR_PROB_TABLE_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"

class PriorProbTable {
private:
    NgramScoreTable* table;
public:
    PriorProbTable(UTF8InputStream& in);
	~PriorProbTable();
    float lookup(Symbol category, Symbol tag) const;
};

#endif
