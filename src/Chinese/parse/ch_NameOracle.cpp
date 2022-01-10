// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/parse/ch_NameOracle.h"
#include "Generic/parse/ParserTags.h"
#include "Chinese/parse/ch_STags.h"

bool ChineseNameOracle::isName(ChartEntry *entry, Symbol chain) const {

	if (!entry->isPreterminal && entry->nameType != ParserTags::nullSymbol)
		return true;
	
	return false;

}
