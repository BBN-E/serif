#ifndef _TEMPORAL_INSTANCE_GENERATOR_H_
#define _TEMPORAL_INSTANCE_GENERATOR_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "LearnIt/MatchProvenance.h"
#include "TemporalInstance.h"

class SentenceTheory;
class ValueMention;
BSP_DECLARE(ElfRelation)
BSP_DECLARE(TemporalAttribute)
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(TemporalInstanceGenerator)

class TemporalInstanceGenerator {
public:
	TemporalInstanceGenerator(TemporalTypeTable_ptr typeTable);
	virtual void instances(const Symbol& docid, const SentenceTheory* st, 
			ElfRelation_ptr relation, TemporalInstances& instances);
	virtual ~TemporalInstanceGenerator();
protected:
	virtual void instancesInternal(const Symbol& docid, const SentenceTheory* st, 
			ElfRelation_ptr relation, TemporalInstances& instances) = 0;
	const TemporalTypeTable_ptr typeTable() const;
	std::wstring makePreviewString(const SentenceTheory* st,
		ElfRelation_ptr relation, unsigned int attributeType, const ValueMention* vm);
private:
	TemporalTypeTable_ptr _typeTable;
	Symbol::SymbolGroup _eligibleRelations;
};

#endif
