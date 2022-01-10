// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_ACRONYMMAKER_H
#define xx_ACRONYMMAKER_H

#include "Generic/edt/AcronymMaker.h"
#include <iostream>


class GenericAcronymMaker: public AcronymMaker {
public:
	GenericAcronymMaker();
	std::vector<Symbol> generateAcronyms(const std::vector<Symbol>& words);
private:
	typedef std::vector<Symbol> SymbolList;
	typedef Symbol::HashMap<std::vector<SymbolList> > ReservedAcronymTable;
	ReservedAcronymTable reservedAcronyms;
};


#endif

