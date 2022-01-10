// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CATEGORY_PROB_DERIVER_H
#define CATEGORY_PROB_DERIVER_H

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"

class CategoryProbDeriver
{
private:
	int nodeCount;
	NgramScoreTable* categoryPriors;

public:
	CategoryProbDeriver(UTF8InputStream& in);
	void derive_tables();
	void print_tables(char* filename);
};

#endif
