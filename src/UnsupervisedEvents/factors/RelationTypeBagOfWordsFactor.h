#ifndef _RELATION_TYPE_BAG_OF_WORDS_FACTOR_H_
#define _RELATION_TYPE_BAG_OF_WORDS_FACTOR_H_

#include "Generic/common/bsp_declare.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include "../../GraphicalModels/Factor.h"
#include "../../GraphicalModels/BagOfWordsFactor.h"
#include "../../GraphicalModels/distributions/MultinomialDistribution.h"

class RelationTypeVariable;
BSP_DECLARE(RelationTypeBagOfWordsFactor)
class Passage;
class SynNode;

typedef GraphicalModel::MultinomialDistribution<GraphicalModel::SymmetricDirichlet>
	SymMulti;
typedef GraphicalModel::BagOfWordsFactor<RelationTypeBagOfWordsFactor,
		RelationTypeVariable, SymMulti> RTBOWFParent;
class RelationTypeBagOfWordsFactor : public RTBOWFParent {
	public:
		RelationTypeBagOfWordsFactor(unsigned int n_relation_types) 
		: RTBOWFParent(n_relation_types) {}
		void gatherWords(const SynNode* node);
/*	private:
		friend class GraphicalModel::BagOfWordsFactor
			<RelationTypeBagOfWordsFactor, RelationTypeVariable, SymMulti>;*/
};

#endif

