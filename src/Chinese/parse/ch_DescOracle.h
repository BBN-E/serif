// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_DESC_ORACLE_H
#define ch_DESC_ORACLE_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/KernelTable.h"

class ChineseDescOracle {
private:

	Symbol _NP_NPPOS_Symbol;
	Symbol _S_NPPOS_Symbol;
	SymbolHash * _significantHeadWords;
	SymbolHash * _npChains;
	
	void readInventory(const char *filename);
	void initialize(KernelTable* kernelTable, 
 				    ExtensionTable* extensionTable);
	
public:
  
	ChineseDescOracle(KernelTable* kernelTable, 
			   ExtensionTable* extensionTable, 
			   const char *inventoryFile);
	bool isNounPhrase(ChartEntry *entry, Symbol chain) const;
	bool isPossibleDescriptor(ChartEntry *entry, Symbol chain) const;	
	bool isPossibleDescriptorHeadWord(Symbol word) const;	

};

#endif



