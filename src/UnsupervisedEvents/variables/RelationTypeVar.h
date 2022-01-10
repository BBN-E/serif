#ifndef _RELATION_TYPE_VAR_H_
#define _RELATION_TYPE_VAR_H_

#include "Generic/common/bsp_declare.h"
#include "../../GraphicalModels/Variable.h"
#include "../Passage.h"

BSP_DECLARE(RelationTypeVariable)
class RelationTypeVariable : public GraphicalModel::Variable {
public:
	RelationTypeVariable(unsigned int n_relation_types) : Variable(n_relation_types) {}
};

#endif

