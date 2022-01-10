// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SC_ORACLE_H
#define SC_ORACLE_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"
#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"

class SignificantConstitOracle {
public:
	/** Create and return a new SignificantConstitOracle. */
	static SignificantConstitOracle *build(KernelTable* kernelTable, 
								   ExtensionTable* extensionTable, 
								   const char *inventoryFile) { 
									   return _factory()->build(kernelTable, extensionTable, inventoryFile); 
	}
	/** Hook for registering new SignificantConstitOracle factories */
	struct Factory { 
		virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
								   ExtensionTable* extensionTable, 
								   const char *inventoryFile) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	/**
	 * tells whether an entire entry is significant or not
	 */
	virtual bool isSignificant(ChartEntry *entry, Symbol chain) const = 0;

	/**
	 *	used to set entry->headIsDescWord
	 *	in English (at least), just passes the call through to the descOracle
	 */
	virtual bool isPossibleDescriptorHeadWord(Symbol word) const = 0;

//	SignificantConstitOracle(KernelTable* kernelTable, 
//		ExtensionTable* extensionTable, const char *inventoryFilename) {}

	virtual ~SignificantConstitOracle() {}

protected:
	SignificantConstitOracle() {}
	SignificantConstitOracle(KernelTable* kernelTable, 
								   ExtensionTable* extensionTable, 
								   const char *inventoryFile) {}



private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/parse/en_SignificantConstitOracle.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/parse/ch_SignificantConstitOracle.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/parse/ar_SignificantConstitOracle.h"
//#else
//	#include "Generic/parse/xx_SignificantConstitOracle.h"
//#endif


#endif




