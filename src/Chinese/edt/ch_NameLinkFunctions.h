// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_NAMELINKFUNCTIONS_H
#define ch_NAMELINKFUNCTIONS_H

#include "Generic/edt/xx_NameLinkFunctions.h"
#include "Generic/common/SymbolHash.h"

class ChineseNameLinkFunctions: public GenericNameLinkFunctions {
public:
	static bool populateAcronyms(const Mention *mention, EntityType linkType);
	static void destroyDataStructures();
	static void recomputeCounts(CountsTable &inTable, 
								CountsTable &outTable, 
								int &outTotalCount);
	static int getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results);

private:	
	static SymbolHash _stopWords;
	static bool _is_initialized;

	static void loadStopWords(const char *file);
};

#endif
