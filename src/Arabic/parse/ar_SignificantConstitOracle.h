// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_SC_ORACLE_H
#define ar_SC_ORACLE_H

#include "Arabic/parse/ar_DescOracle.h"
#include "Arabic/parse/ar_NameOracle.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/SignificantConstitOracle.h"


class ArabicSignificantConstitOracle : public SignificantConstitOracle {
private:
	friend class ArabicSignificantConstitOracleFactory;

	const ArabicDescOracle * descOracle;
	const ArabicNameOracle * nameOracle;
	bool hasSignificantPP(ChartEntry *entry) const;

public:
	/**
	 * tells whether an entire entry is significant or not
	 */
	virtual bool isSignificant(ChartEntry *entry, Symbol chain) const;
	
	/**
	 *	used to set entry->headIsDescWord
	 *  Note: English uses a list of possibly signficant words
	 *		until such a list is created for arabic, entry->headIsSignificant
	 *		is useless (it will be true for names and parents of names, but
	 *		false otherwise).  For now all NP's are significant!
	 */
	virtual bool isPossibleDescriptorHeadWord(Symbol word) const {
		return false;
	}
	
private:
	ArabicSignificantConstitOracle(KernelTable* kernelTable, 
								   ExtensionTable* extensionTable, 
								   const char *inventoryFile);

};

class ArabicSignificantConstitOracleFactory: public SignificantConstitOracle::Factory {
	virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, 
		const char *inventoryFile) { 
			return _new ArabicSignificantConstitOracle(kernelTable, extensionTable, inventoryFile); 
	}
};


#endif




