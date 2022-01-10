// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_DESC_ORACLE_H
#define ar_DESC_ORACLE_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/KernelTable.h"

class ArabicDescOracle {
private:
	SymbolHash * _npChains;
public:
  
	ArabicDescOracle(KernelTable* kernelTable, ExtensionTable* extensionTable);
	bool isNounPhrase(ChartEntry *entry, Symbol chain) const;
	bool isPossibleDescriptor(ChartEntry *entry, Symbol chain) const;	
	bool isPossibleDescriptorHeadWord(Symbol word) const;

};

#endif
