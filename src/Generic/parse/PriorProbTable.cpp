// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/PriorProbTable.h"
#include "Generic/common/Symbol.h"

PriorProbTable::PriorProbTable(UTF8InputStream& in)
{
    table =_new NgramScoreTable(2, in);
}
PriorProbTable::~PriorProbTable() {
	delete table;
}

float PriorProbTable::lookup(Symbol category, Symbol tag) const
{
	Symbol ngram[2];
	ngram[0] = category; ngram[1] = tag;
    float result = table->lookup(ngram);
	if (result == 0)
		result = -10000;
	return result;
}
