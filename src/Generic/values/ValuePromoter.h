// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_PROMOTER_H
#define VALUE_PROMOTER_H

#include "Generic/common/limits.h"
#include "Generic/common/DebugStream.h"

class DocTheory;
class Value;

class ValuePromoter {
public:
	ValuePromoter();
	void promoteValues(DocTheory* docTheory);

	DebugStream _debugStream;

private:	
	Value *_valueList[MAX_DOCUMENT_VALUES];
	int _n_values;
};

#endif
