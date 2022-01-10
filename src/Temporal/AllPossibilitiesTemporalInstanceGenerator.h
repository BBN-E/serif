#ifndef _ALL_POSS_GEN_
#define _ALL_POSS_GEN_

#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "TemporalInstance.h"
#include "TemporalInstanceGenerator.h"

class SentenceTheory;
BSP_DECLARE(ElfRelation)
BSP_DECLARE(TemporalTypeTable)

class AllPossibilitiesTemporalInstanceGenerator : public TemporalInstanceGenerator  {
public:
	AllPossibilitiesTemporalInstanceGenerator(TemporalTypeTable_ptr typeTable);
protected:
	virtual void instancesInternal(const Symbol& docid, const SentenceTheory* st, 
			ElfRelation_ptr relation, TemporalInstances& instances);
};

#endif

