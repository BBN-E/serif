// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_SC_ORACLE_H
#define ES_SC_ORACLE_H

#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/SignificantConstitOracle.h"


class SpanishSignificantConstitOracle : public SignificantConstitOracle {
private:
	friend class SpanishSignificantConstitOracleFactory;

public:
	/**
	 * tells whether an entire entry is significant or not
	 */
	virtual bool isSignificant(ChartEntry *entry, Symbol chain) const;
	
	/**
	 *	used to set entry->headIsDescWord
	 */
	virtual bool isPossibleDescriptorHeadWord(Symbol word) const;
	
private:
	SpanishSignificantConstitOracle(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, const char *inventoryFilename);

	bool hasSignificantPP(ChartEntry *entry) const;

};

class SpanishSignificantConstitOracleFactory: public SignificantConstitOracle::Factory {
	virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, 
		const char *inventoryFile) { return _new SpanishSignificantConstitOracle(kernelTable, extensionTable, inventoryFile); }
};


#endif




