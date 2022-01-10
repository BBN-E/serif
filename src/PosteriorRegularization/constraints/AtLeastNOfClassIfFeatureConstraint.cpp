#include "AtLeastNOfClassIfFeatureConstraint.h"

#include <boost/foreach.hpp>
#include "../PosteriorRegularization.h"
#include "../../GraphicalModels/DataSet.h"
#include "../../GraphicalModels/Alphabet.h"

using std::vector;
using std::wstring;
using GraphicalModel::DataSet;
using GraphicalModel::Alphabet;

AtLeastNOfClassIfFeatureConstraint::AtLeastNOfClassIfFeatureConstraint(
		const DataSet<Document>& graphs,
	const vector<unsigned int> classes, const vector<wstring>& features, 
	double N)
	   : _classes(classes), _marked(graphs.nFactors()), _N(N),
	   _cachedBounds(graphs.nGraphs())
{
	vector<unsigned int> resolvedFeatures;
   BOOST_FOREACH(const wstring& featName, features) {
	   resolvedFeatures.push_back(BMMFeatureFactor::featureIndex(featName));
   }
   cache(graphs, resolvedFeatures, N);
}

void AtLeastNOfClassIfFeatureConstraint::updateBoundAndExpectation(const Document& graph) {
	_bound = _cachedBounds[graph.id];
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		if (_marked[featureFactor->id]) {
			BOOST_FOREACH(unsigned int cls, _classes) {
				_expectation += featureFactor->variable().marginals()[cls];
			}
		}
	}
}

void AtLeastNOfClassIfFeatureConstraint::modifyFactors(Document& graph) const {
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph.featureFactors) {
		if (_marked[featureFactor->id]) {
			BOOST_FOREACH(unsigned int cls, _classes) {
				featureFactor->modifiers()[cls] += _weight;
			}
		}
	}
}

void AtLeastNOfClassIfFeatureConstraint::cache(
		const DataSet<Document>& graphs, const vector<unsigned int>& features,
		double threshold)
{
	BOOST_FOREACH(const Document_ptr& graph, graphs.graphs) {
		bool found_feature = false;
		BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, graph->featureFactors) {
			_marked[featureFactor->id] = false;
			BOOST_FOREACH(unsigned int feature, features) {
				const std::vector<unsigned int>& fv = 
					featureFactor->features();
				if (find(fv.begin(), fv.end(), feature) != fv.end())
				{
					_marked[featureFactor->id] = true;
					found_feature = true;
					break;
				}
			}
		}
		if (found_feature) {
			_cachedBounds[graph->id] = threshold;
		} else {
			_cachedBounds[graph->id] = 0.0;
		}
	}
}

