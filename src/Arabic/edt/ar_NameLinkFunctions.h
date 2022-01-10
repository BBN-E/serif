// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_NAMELINKFUNCTIONS_H
#define ar_NAMELINKFUNCTIONS_H

#include "Generic/edt/CountsTable.h"
#include "Generic/edt/xx_NameLinkFunctions.h"
class EntitySet;
class Entity;
class DebugStream;

class ArabicNameLinkFunctions: public GenericNameLinkFunctions {
public:
	//eventually use to deal with inflection
	static bool populateAcronyms(const Mention *mention, EntityType linkType);
	static void destroyDataStructures() {};
	static void recomputeCounts(CountsTable &inTable, 
								CountsTable &outTable, 
								int &outTotalCount);
	static int  getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) ;

	//only used in Arabic (i.e., not defined in NameLinkFunctions):
	static EntitySet* mergeEntities(EntitySet* entities);

	static DebugStream &_debugOut; 
private:
	static bool _mentionsMatch(Entity* e1, Entity* e2, EntitySet* entSet);

};

#endif
