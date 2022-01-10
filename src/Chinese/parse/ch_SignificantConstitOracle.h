// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_SC_ORACLE_H
#define ch_SC_ORACLE_H

#include "Chinese/parse/ch_DescOracle.h"
#include "Chinese/parse/ch_NameOracle.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/SignificantConstitOracle.h"


class ChineseSignificantConstitOracle : public SignificantConstitOracle {
private:
	friend class ChineseSignificantConstitOracleFactory;

	const ChineseDescOracle * descOracle;
	const ChineseNameOracle * nameOracle;
	bool hasSignificantPP(ChartEntry *entry) const;

public:
	/**
	 * tells whether an entire entry is significant or not
	 */
	virtual bool isSignificant(ChartEntry *entry, Symbol chain) const;
	
	/**
	 *	used to set entry->headIsDescWord
	 */
	virtual bool isPossibleDescriptorHeadWord(Symbol word) const {
		return descOracle->isPossibleDescriptorHeadWord(word); 
	}
	
private:
	ChineseSignificantConstitOracle(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, const char *inventoryFilename);



};

class ChineseSignificantConstitOracleFactory: public SignificantConstitOracle::Factory {
	virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, 
		const char *inventoryFile) { return _new ChineseSignificantConstitOracle(kernelTable, extensionTable, inventoryFile); }
};


#endif




