#ifndef _CLASS_REQUIRES_FEATURE_H_
#define _CLASS_REQUIRES_FEATURE_H_

#include "Constraint.h"
#include <vector>
#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/SessionLogger.h"
#include "GraphicalModels/Alphabet.h"

namespace GraphicalModel {
	template <typename GT> class DataSet;
	class ProblemDefinition;

template <typename View>
class ClassRequiresFeature : public GraphicalModel::Constraint<typename View::GraphType> 
{
public:
	typedef boost::shared_ptr<typename View::FactorType> FactorType_ptr;
	typedef std::vector<FactorType_ptr> Factors;
	typedef std::vector<std::wstring> Strings;
	typedef GraphicalModel::DataSet<typename View::GraphType> Data;
	typedef std::vector<unsigned int> FV;
	typedef std::vector<unsigned int> Classes;

	ClassRequiresFeature(const Data& graphs, const Strings& features,
			const Classes& classes, double threshold, const View& adaptor) 
		: GraphicalModel::Constraint<typename View::GraphType>(L">="), _classes(classes), 
		_marked(graphs.nFactors()), _view(adaptor), _cachedBounds(graphs.nGraphs()),
		_resolvedFeatures()
	{
		Strings::const_iterator fIt = features.begin();
		for (; fIt!=features.end(); ++fIt) {
			try {
				_resolvedFeatures.push_back(View::FactorType::featureIndex(*fIt));
			} catch (const BadLookupException&) {
				SessionLogger::warn("feature_not_found") << L"Feature " 
					<< *fIt << L" used in constraint is unknown";
			}
		}
		cache(graphs, threshold);
	}

	void modifyFactors(typename View::GraphType& graph) const {
		for (size_t fact = 0; fact < _view.nFactors(graph); ++fact) {
			typename View::FactorType& factor = _view.factor(graph, fact);
			if (_marked[factor.id]) {
				for (typename Classes::const_iterator cIt = _classes.begin(); cIt!=_classes.end(); ++cIt) {
					factor.adjustModifier(*cIt, -this->_weight, 
							const_cast<ClassRequiresFeature*>(this));
				}
			}
		}
	}

	void dumpFactorModifications(const typename View::GraphType& g, std::wostream& out) const {
		bool got_anything = false;

		for (size_t fact = 0; fact < _view.nFactors(g); ++fact) {
			const typename View::FactorType& factor = _view.factor(g, fact);
			if (_marked[factor.id]) {
				for (typename Classes::const_iterator cIt = _classes.begin(); cIt!=_classes.end(); ++cIt) {
					if (!got_anything) {
						got_anything = true;
						out << L"\tConstraint " << name();
						out << L"Expectation: " << calcExpectation(g)
							<< L"; Bounds: " << _cachedBounds[g.id] << L"\n";
					}
					out << L"\t\tClass " << *cIt << L" of variable "
						<< fact << L" adjusted by " << -this->_weight << L"\n";
				}
			}
		}
	}

	std::wstring name() const {
		std::wstringstream str;

		str << L"RequiresFeature(";
		DumpUtils::dumpFeatureVector<View>(_resolvedFeatures, str);
		str << L" => ";
		DumpUtils::dumpIntVector(_classes, str);
		str << L")";

		return str.str();
	}

	void updateBoundAndExpectation(const typename View::GraphType& graph) {
		this->_bound += _cachedBounds[graph.id];
		this->_expectation += calcExpectation(graph);
	}

	double debugViolation(const typename View::GraphType& graph) const {
		return _cachedBounds[graph.id] - calcExpectation(graph);
	}

	void contributeToLogZ(const typename View::GraphType& graph,
			unsigned int var, std::vector<double>& scores) const
	{
		const typename View::FactorType& factor = _view.factorForVar(graph, var);
		if (_marked[factor.id]) {
			for (typename Classes::const_iterator cIt = _classes.begin();
					cIt!=_classes.end(); ++cIt)
			{
				scores[*cIt] -= this->_weight;
			}
		}
	}

	void relax(const typename View::GraphType& graph) {
		_cachedBounds[graph.id] = 0.0;
		for (size_t fact = 0; fact<_view.nFactors(graph); ++fact) {
			const typename View::FactorType& factor = _view.factor(graph, fact);
			_marked[factor.id] = false;
		}
	}

private:
	std::vector<bool> _marked;
	Classes _classes;
	std::vector<double> _cachedBounds;
	View _view;
	FV _resolvedFeatures;

	double calcExpectation(const typename View::GraphType& graph) const {
		double expectation = 0.0;
		for (size_t fact = 0; fact<_view.nFactors(graph); ++fact) {
			const typename View::FactorType& factor = _view.factor(graph, fact);
			if (_marked[factor.id]) {
				for (typename Classes::const_iterator cIt = _classes.begin(); cIt!=_classes.end(); ++cIt) {
					expectation -= _view.marginals(graph, fact)[*cIt];
				}
			}
		}
		return expectation;
	}

	double cacheBound(const typename View::GraphType& graph, double threshold) 
	{
		double bound = 0.0;
		for (size_t fact = 0; fact<_view.nFactors(graph); ++fact) {
			const typename View::FactorType& factor = _view.factor(graph, fact);
			if (_marked[factor.id]) {
				bound -= threshold;
			}
		}
		return bound;
	}

	void cache(const Data& graphs, double threshold)
	{
		typename Data::GraphVector::const_iterator gIt = graphs.graphs.begin();
		for (; gIt != graphs.graphs.end(); ++gIt) {
			for (size_t fact = 0; fact<_view.nFactors(**gIt); ++fact) {
				const typename View::FactorType& factor = _view.factor(**gIt, fact);
				_marked[factor.id] = true;
				typename FV::const_iterator featIt = _resolvedFeatures.begin();
				for (; featIt!=_resolvedFeatures.end(); ++featIt) {
					if (factor.hasFeature(*featIt)) {
						_marked[factor.id] = false;
						break;
					}
				}
			}
		}

		for (unsigned int i=0; i<_cachedBounds.size(); ++i) {
			_cachedBounds[i] = cacheBound(*graphs.graphs[i], threshold);
		}
	}
};
}

#endif

