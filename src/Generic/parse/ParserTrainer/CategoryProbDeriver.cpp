// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/ParserTrainer/CategoryProbDeriver.h"
#include "Generic/common/UTF8InputStream.h"
#include <math.h>


CategoryProbDeriver::CategoryProbDeriver(UTF8InputStream& in)
{
	in >> nodeCount;
	categoryPriors = _new NgramScoreTable(2, in);
}

void
CategoryProbDeriver::derive_tables()
{
	NgramScoreTable::Table::iterator iter;

	for (iter = categoryPriors->get_start(); iter != categoryPriors->get_end(); ++iter) {
		(*iter).second = log ((*iter).second / nodeCount);
		
	}

}

void
CategoryProbDeriver::print_tables(char* filename)
{
	categoryPriors->print(filename);
}
