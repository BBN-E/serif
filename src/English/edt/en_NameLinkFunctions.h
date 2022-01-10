// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_NAMELINKFUNCTIONS_H
#define en_NAMELINKFUNCTIONS_H

#include "Generic/edt/xx_NameLinkFunctions.h"

class EnglishNameLinkFunctions: public GenericNameLinkFunctions {
public:
	static bool populateAcronyms(const Mention *mention, EntityType type);
	static void destroyDataStructures() {};
	static void recomputeCounts(CountsTable &inTable, 
								CountsTable &outTable, 
								int &outTotalCount);
};

#endif
