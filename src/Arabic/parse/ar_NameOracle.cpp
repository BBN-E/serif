// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_NameOracle.h"
#include "Generic/parse/ParserTags.h"
#include "Arabic/parse/ar_STags.h"

bool ArabicNameOracle::isName(ChartEntry *entry, Symbol chain) const {

	if (!entry->isPreterminal && entry->nameType != ParserTags::nullSymbol)
		return true;
	return false;

}
