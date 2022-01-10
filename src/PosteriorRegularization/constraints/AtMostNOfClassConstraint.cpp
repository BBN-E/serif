#include "AtMostNOfClassConstraint.h"

#include <boost/foreach.hpp>
#include "../PosteriorRegularization.h"

using std::vector;

AtMostNOfClassConstraint::AtMostNOfClassConstraint(
const std::vector<unsigned int>& classes, double N) 
	: _classes(classes), _N(N) {}

void AtMostNOfClassConstraint::updateBoundAndExpectation(const Document& graph) {
	// note the "-=" in the bound and expectation updates
	// Those make this an "at most" constraint
	_bound -= _N;
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		BOOST_FOREACH(unsigned int cls, _classes) {
			_expectation -= featureFactor->variable().marginals()[cls];
		}
	}
}

void AtMostNOfClassConstraint::modifyFactors(Document& graph) const {
	// note -=, see above
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		BOOST_FOREACH(unsigned int cls, _classes) {
			featureFactor->modifiers()[cls] -= _weight;
		}
	}
}

