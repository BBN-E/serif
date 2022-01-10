#include "ClassRequiresFeatureConstraint.h"

#include <boost/foreach.hpp>
#include "../PosteriorRegularization.h"
#include "../../GraphicalModels/DataSet.h"
#include "../../GraphicalModels/Alphabet.h"

using std::vector;
using std::wstring;
using GraphicalModel::DataSet;
using GraphicalModel::Alphabet;

ClassRequiresFeatureConstraint::ClassRequiresFeatureConstraint(
		const DataSet<Document>& graphs,
		const vector<wstring>& features,
		const vector<unsigned int>& classes, double threshold)
: _classes(classes), _marked(graphs.nFactors())
{
	vector<unsigned int> resolvedFeatures;
	BOOST_FOREACH(const wstring& f, features) {
		resolvedFeatures.push_back(BMMFeatureFactor::featureIndex(f));
	}
	cache(graphs, resolvedFeatures, threshold);
}

void ClassRequiresFeatureConstraint::updateBoundAndExpectation(const Document& graph) {
	// we don't update the bound because this constraint has a constant 0 bound
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		double p = phi(featureFactor->id);
		BOOST_FOREACH(unsigned int cls, _classes) {
			_expectation += p*featureFactor->variable().marginals()[cls];
		}
	}
}

void ClassRequiresFeatureConstraint::cache(
		const DataSet<Document>& graphs, const vector<unsigned int>& features, 
		double threshold)
{
	BOOST_FOREACH(const Document_ptr& graph, graphs.graphs) {
		BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph->featureFactors) {
			_marked[featureFactor->id] = false;
			BOOST_FOREACH(unsigned int feature, features) {
				const std::vector<unsigned int>& fv = featureFactor->features();
				if (find(fv.begin(), fv.end(), feature) != fv.end())
				{
					_marked[featureFactor->id] = true;
					break;
				}
			}
		}
	}
}

void ClassRequiresFeatureConstraint::modifyFactors(Document& graph) const {
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		double p = phi(featureFactor->id);
		BOOST_FOREACH(unsigned int cls, _classes) {
			featureFactor->modifiers()[cls] += _weight * p;
		}
	}
}

double ClassRequiresFeatureConstraint::phi(unsigned int factor_id) const {
	return -0.8 + indicator(_marked[factor_id]);
}

