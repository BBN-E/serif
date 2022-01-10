#ifndef _VARIABLE_PRIOR_FACTOR_H_
#define _VARIABLE_PRIOR_FACTOR_H_

#include <vector>
#include <cmath>
#include <iostream>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "Factor.h"
#include "Alphabet.h"
#include "VectorUtils.h"
#include "distributions/BernoulliDistribution.h"

namespace GraphicalModel {

template <typename GraphType, typename FactorType, 
		 typename VariableType, typename Distribution>
class VariablePriorFactor : public ModifiableUnaryFactor<GraphType, FactorType> {
	public:
		VariablePriorFactor(unsigned int n_classes)
			: GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>(n_classes) {}

		static void finishedLoading(unsigned int num_var_vals,
				const boost::shared_ptr<typename Distribution::Prior>& prior
			/*	, 
				const GraphicalModel::Alphabet& alphabet*/)
		{
			_probs = boost::make_shared<Distribution>(num_var_vals, prior);
		}

		static void clearCounts() {
			_probs->reset();
		}

		static void observe(const VariableType& v) 
		{
			assert(v.nValues() == _probs->size());
			for (size_t cls =0; cls < _probs->size(); ++cls) {
				_probs->observeInstance(cls, v.marginals()[cls]);
			}
		}

		template <typename Dumper>
		static void updateParametersFromCounts(Dumper& dumper) {
			_probs->digestObservations();
			_probs->toHTML(dumper.stream());
			_probs->toLogSpace();
		}

	private:
		static boost::shared_ptr<Distribution> _probs;

		friend class GraphicalModel::ModifiableUnaryFactor<GraphType, FactorType>;
		double factorPreModImpl(unsigned int assignment) const {
			return (*_probs)(assignment);
		}
};


template <typename GraphType, typename FactorType, typename VariableType, typename Distribution>
boost::shared_ptr<Distribution> VariablePriorFactor<GraphType, FactorType,
	VariableType, Distribution>::_probs;

};

#endif

