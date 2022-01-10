// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_NAMELINKFUNCTIONS_H
#define es_NAMELINKFUNCTIONS_H

#include "Generic/edt/xx_NameLinkFunctions.h"

class SpanishNameLinkFunctions: public GenericNameLinkFunctions {
public:
	static bool populateAcronyms(const Mention *mention, EntityType linkType);
	static void destroyDataStructures() {}
	static void recomputeCounts(CountsTable &inTable, 
								CountsTable &outTable, 
								int &outTotalCount);
};

#endif
