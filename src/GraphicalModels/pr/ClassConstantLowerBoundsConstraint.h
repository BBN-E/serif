#ifndef _CLASS_CONSTANT_LOWER_BOUND_CONSTRAINT_H_
#define _CLASS_CONSTANT_LOWER_BOUND_CONSTRAINT_H_

#include "Constraint.h"
#include <vector>
#include <sstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include "Generic/common/SessionLogger.h"

namespace GraphicalModel {
	template <typename GT> class DataSet;

template <typename View>
class ClassConstantLowerBoundConstraint : public GraphicalModel::Constraint<typename View::GraphType>{
public:
	typedef std::vector<unsigned int> Classes;

	ClassConstantLowerBoundConstraint(const typename View::GraphType& graph, 
			const Classes& classes, double bound, const View& adaptor) 
		: GraphicalModel::Constraint<typename View::GraphType>(L"<="),
		_classes(classes), _view(adaptor), _constant_bound(bound), _active(true)
	{ }

	void modifyFactors(typename View::GraphType& graph) const {
		if (_active) {
			for (size_t fact = 0; fact < _view.nFactors(graph); ++fact) {
				typename View::FactorType& factor = _view.factor(graph, fact);
				for (typename Classes::const_iterator cIt = _classes.begin(); cIt!=_classes.end(); ++cIt) {
					factor.adjustModifier(*cIt, this->_weight, 
							const_cast<ClassConstantLowerBoundConstraint*>(this));
				}
			}
		}
	}

	void dumpFactorModifications(const typename View::GraphType& graph, std::wostream& out) const {
		if (_active) {
			bool got_anything = false;
			for (size_t fact = 0; fact < _view.nFactors(graph); ++fact) {
				const typename View::FactorType& factor = _view.factor(graph, fact);
				for (typename Classes::const_iterator cIt = _classes.begin(); cIt!=_classes.end(); ++cIt) {
					if (!got_anything) {
						got_anything = true;
						out << L"\tConstraint " << name() ;
						out << L"Expectation: " << calcExpectation(graph)
							<< L"; Bounds: " << _constant_bound << L"\n";
					}
					out << L"\t\tClass " << *cIt << L" for variable " << fact
						<< L" adjusted by " << this->_weight << L"\n";
				}
			}
		}
	}

	std::wstring name() const {
		std::wstringstream str;

		str << L"LowerBound(";
		DumpUtils::dumpIntVector(_classes, str);
		str << L" >= " << _constant_bound << L")";
		if (!_active) {
			str << L" INACTIVE";
		}

		return str.str();
	}

	void updateBoundAndExpectation(const typename View::GraphType& graph) {
		if (_active) {
			// bound is constant
			this->_bound += _constant_bound;
			this->_expectation += calcExpectation(graph);
		}
	}

	double debugViolation(const typename View::GraphType& graph) const {
		return _constant_bound - calcExpectation(graph);
	}

	void contributeToLogZ(const typename View::GraphType& graph,
			unsigned int var, std::vector<double>& scores) const
	{
		if (_active) {
			for (typename Classes::const_iterator cIt = _classes.begin();
					cIt!=_classes.end(); ++cIt)
			{
				scores[*cIt] += this->_weight;
			}
		}
	}

	void relax(const typename View::GraphType& graph) {
		_active = false;
	}
private:
	Classes _classes;
	double _constant_bound;
	View _view;
	bool _active;

	double calcExpectation(const typename View::GraphType& graph) const {
		double expectation = 0.0;
		for (size_t fact = 0; fact < _view.nFactors(graph); ++fact) {
			for (typename Classes::const_iterator cIt = _classes.begin();
					cIt!=_classes.end(); ++cIt)
			{
				expectation += _view.marginals(graph, fact)[*cIt];
			}
		}
		return expectation;
	}
	
};
}


#endif

