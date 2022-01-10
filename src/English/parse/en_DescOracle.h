// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_DESC_ORACLE_H
#define en_DESC_ORACLE_H

#include <fstream>
#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/wordnet/xx_WordNet.h"

class EnglishDescOracle {
private:

	Symbol _NP_NPPOS_Symbol;
	Symbol _S_NPPOS_Symbol;
	SymbolHash * _significantHeadWords;
	SymbolHash * _npChains;
	WordNet * _wordNet;
	
	Symbol lowercaseSymbol(Symbol s) const;
	void readInventory(const char *filename);
	void initialize(KernelTable* kernelTable, 
 				    ExtensionTable* extensionTable);
	
public:
    
	bool isPartitive(ChartEntry *entry) const {
		return _wordNet->isPartitive(entry);
	}
	EnglishDescOracle(KernelTable* kernelTable, ExtensionTable* extensionTable,
					   const char *inventoryFile);
	bool isNounPhrase(ChartEntry *entry, Symbol chain) const;
	bool isPossibleDescriptor(ChartEntry *entry, Symbol chain) const;
	bool isPossibleDescriptorHeadWord(Symbol word) const;		

};

#endif



