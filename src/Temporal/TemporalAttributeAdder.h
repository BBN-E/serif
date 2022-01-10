#ifndef _TEMPORAL_ATTRIBUTE_ADDER_H_
#define _TEMPORAL_ATTRIBUTE_ADDER_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
//#include "LearnIt/Eigen/Core"

BSP_DECLARE(ElfRElation)
BSP_DECLARE(TemporalAttribute)
BSP_DECLARE(ElfRelation)
BSP_DECLARE(ElfIndividual)
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(TemporalDB)
BSP_DECLARE(TemporalAttributeAdder)
BSP_DECLARE(TemporalFeature)
BSP_DECLARE(FeatureMap)
BSP_DECLARE(TemporalInstanceGenerator)
BSP_DECLARE(TemporalInstance)
class ValueMention;
	
class DocTheory;

class TemporalAttributeAdder {
public:
	TemporalAttributeAdder(TemporalDB_ptr db);
	virtual void addTemporalAttributes(ElfRelation_ptr relation, 
			const DocTheory* dt, int sn) = 0;
	virtual ~TemporalAttributeAdder();
protected:
	TemporalTypeTable_ptr _typeTable;
	TemporalInstanceGenerator_ptr _instanceGenerator;
	TemporalDB_ptr _db;
	std::vector<TemporalFeature_ptr> _features;

	bool _record_temporal_source;
	bool _debug_matching;

	void addAttributeToRelation(ElfRelation_ptr relation,
		TemporalAttribute_ptr attribute, const std::wstring& scoreScore,
		const DocTheory* doc_theory, const std::wstring& proposalSource);
	double scoreInstance(TemporalInstance_ptr inst, const DocTheory* dt, int sn);
	bool relationAlreadyContainsVM(ElfRelation_ptr relation, const ValueMention* vm);

	bool doAffiliatedHack(TemporalInstance_ptr inst);
	void undoAffiliatedHack(TemporalInstance_ptr inst);

	void stripSuffixes(ElfIndividual_ptr individual);
	bool convertWeek(ElfIndividual_ptr individual);
};

#endif
