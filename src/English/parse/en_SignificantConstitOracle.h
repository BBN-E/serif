// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_SC_ORACLE_H
#define en_SC_ORACLE_H

#include "English/parse/en_DescOracle.h"
#include "English/parse/en_NameOracle.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/SignificantConstitOracle.h"


class EnglishSignificantConstitOracle : public SignificantConstitOracle {
private:
	friend class EnglishSignificantConstitOracleFactory;

	const EnglishDescOracle * descOracle;
	const EnglishNameOracle * nameOracle;
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
	EnglishSignificantConstitOracle(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, const char *inventoryFile);



};

class EnglishSignificantConstitOracleFactory: public SignificantConstitOracle::Factory {
	virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, 
		const char *inventoryFile) { return _new EnglishSignificantConstitOracle(kernelTable, extensionTable, inventoryFile); }
};


#endif




