// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_SC_ORACLE_H
#define xx_SC_ORACLE_H

#include "Generic/parse/SignificantConstitOracle.h"

class GenericSignificantConstitOracle : public SignificantConstitOracle{
private:
	friend class GenericSignificantConstitOracleFactory;

	static void defaultMsg(){
		std::cerr<<"<<<<<<<<<<<WARNING: Using unimplemented Significant Constit Oracle!>>>>>\n";
	}
public:
	virtual bool isSignificant(ChartEntry *entry, Symbol chain) const{
		defaultMsg();
		return false;
	}
	virtual bool isPossibleDescriptorHeadWord(Symbol word) const{
		defaultMsg();
		return false;
	}

private:
	GenericSignificantConstitOracle(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, const char *inventoryFilename) {
		defaultMsg();
	}
	//GenericSignificantConstitOracle(){
	//	defaultMsg();
	//}
};

class GenericSignificantConstitOracleFactory: public SignificantConstitOracle::Factory {
	virtual SignificantConstitOracle *build(KernelTable* kernelTable, 
		ExtensionTable* extensionTable, 
		const char *inventoryFile) { 
			return _new GenericSignificantConstitOracle(kernelTable, extensionTable, inventoryFile); 
	}
};

#endif


