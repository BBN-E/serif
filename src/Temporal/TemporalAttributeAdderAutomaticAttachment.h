#ifndef _TEMPORAL_ATTRIBUTE_ADDER_AUTOMATIC_H_
#define _TEMPORAL_ATTRIBUTE_ADDER_AUTOMATIC_H_

#include "Generic/common/bsp_declare.h"
//#include "learnit/Eigen/Core"
#include "Temporal/TemporalAttributeAdder.h"

BSP_DECLARE(ElfRElation)
BSP_DECLARE(TemporalAttribute)
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(TemporalFeature)
BSP_DECLARE(FeatureMap)
BSP_DECLARE(TemporalInstanceGenerator)
BSP_DECLARE(TemporalInstance)
	
BSP_DECLARE(TemporalAttributeAdderAutomatic)

class TemporalAttributeAdderAutomatic : public TemporalAttributeAdder {
public:
	TemporalAttributeAdderAutomatic(TemporalDB_ptr db);
	void addTemporalAttributes(ElfRelation_ptr relation, const DocTheory* dt, int sn);
private:
	double _threshold;
};

#endif
