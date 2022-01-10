#ifndef _ACE_ENTITY_VARIABLE_H_
#define _ACE_ENTITY_VARIABLE_H_

#include "Generic/common/bsp_declare.h"
#include "GraphicalModels/FactorGraphNode.h"
#include "GraphicalModels/Variable.h"

class DocTheory;
class Entity;
class ProblemDefinition;
BSP_DECLARE(ACEEntityVariable)
class ACEEntityVariable : public GraphicalModel::Variable {
	public:
		ACEEntityVariable(unsigned int n_relation_types) 
			: Variable(n_relation_types), _gold(-1) {}
		void setFromGold(const DocTheory* dt, const Entity* e,
				const ProblemDefinition& problem, unsigned int start_sentence,
				unsigned int end_sentence);
		int gold() const { return _gold; }
		static int getFirstGoldLabel(const DocTheory* dt, const Entity* e,
			const ProblemDefinition& problem, unsigned int start_sentence,
			unsigned int end_sentence);
	private:
		int _gold;
};
#endif

