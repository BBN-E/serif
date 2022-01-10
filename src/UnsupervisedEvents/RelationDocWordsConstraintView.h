#ifndef _RELATION_DOCS_WORDS_CONSTRAINT_ADAPTOR_H_
#define _RELATION_DOCS_WORDS_CONSTRAINT_ADAPTOR_H_

#include <vector>
#include "Passage.h"
#include "factors/RelationTypeBagOfWordsFactor.h"

class RelationDocWordsConstraintView {
	public:
		typedef RelationTypeBagOfWordsFactor FactorType;
		typedef Passage GraphType;

		/*factor_iterator factorsBegin(Passage& p) const {
			return p.beginRelWordFactors();
		}*/

		size_t nFactors(const Passage& p) const {
			return 1;
		}

		const FactorType& factor(const Passage& p, size_t i) const {
			assert(i == 0);
			return p.documentFactor();
		}

		const FactorType& factorForVar(const Passage& p, size_t var) const {
			return factor(p, var);
		}

		FactorType& factor(Passage& p, size_t i) const {
			assert(i == 0);
			return p.documentFactor();
		}

		const std::vector<double>& marginals(const Passage& p, 
				size_t factor_idx) const 
		{
			assert(factor_idx == 1);
			return p.relationVar().marginals();
		}
};

#endif

