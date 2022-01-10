// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_NameOracle.h"
#include "Generic/parse/ParserTags.h"
#include "English/parse/en_STags.h"

bool EnglishNameOracle::isName(ChartEntry *entry, Symbol chain) const {

	if (!entry->isPreterminal && entry->nameType != ParserTags::nullSymbol)
		return true;
	
	if (entry->constituentCategory == EnglishSTags::ADJP) {
		if (entry->rightChild->headIsSignificant)
			return true;
	}
	
	return false;

}
