#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"

namespace GraphicalModel {
// by default, assumes the constraint has a single weight and expectation
template <typename GraphType>
class Constraint {
public:
	Constraint(const std::wstring& inequality) 
		: _inequality(inequality), _weight(0.0), _expectation(0.0), _bound(0.0) {}
	typedef boost::shared_ptr<Constraint<GraphType> > ptr;

	void clearWeight() {_weight = 0.0; };
	double weight() const { return _weight; };
	void setWeight(double w) { _weight = w;}
	void clearBoundAndExpectation() { _bound = _expectation = 0.0; }

	virtual void updateBoundAndExpectation(const GraphType& graph) = 0;

	void dumpWeight(std::ostream& out) const {
			out << weight();
	}

	void dumpBoundAndExpectation(std::wostream& out) const {
		out << _expectation << L" of " << _bound;
	}

	virtual double gradient() const { 
		return _bound - _expectation; 
	}

	void updateByProjectedGradient(double learning_rate) {
		_weight += learning_rate * (-_bound - _expectation);
		if (_weight < 0.0) {
			_weight = 0.0;
		}
	}

	double bound() const { return _bound; }
	double expectation() const { return _expectation; }
		
	virtual void modifyFactors(GraphType& doc) const = 0;

	virtual void contributeToLogZ(const GraphType& graph, unsigned int var, 
			std::vector<double>& scores) const = 0;

	static double indicator(bool condition) { return condition?1.0:0.0;}

	void dumpDebugInfo(std::wostream& out) {
		out << expectation() << _inequality << bound() << L"; wt="
			<< weight() << L"; grad=" << gradient();
	}
	virtual double debugViolation(const GraphType& graph) const = 0; 

	virtual void dumpFactorModifications(const GraphType& g, std::wostream& out) const = 0;
	virtual std::wstring name() const = 0;
	virtual void relax(const GraphType& graph) = 0;
protected:
	double _weight, _expectation, _bound;
	std::wstring _inequality;
};

template <typename GraphType>
class InstanceConstraintsCollection {
public:
	typedef boost::shared_ptr<Constraint<GraphType> > Constraint_ptr;
	typedef std::vector<Constraint_ptr> ConstraintsList;
	typedef boost::shared_ptr<ConstraintsList> ConstraintsList_ptr;
	typedef boost::shared_ptr<InstanceConstraintsCollection> ptr;

	InstanceConstraintsCollection(unsigned int nInstances, 
			unsigned int nCorpusConstraints) 
		: _instToConst(nInstances), _frozen(false) , 
		_offset(nCorpusConstraints), _instOffsets(nInstances)
	{
		for (size_t i=0; i<_instToConst.size(); ++i) {
			_instToConst[i] = boost::make_shared<ConstraintsList>();
		}
	}

	void addConstraintForInstance(unsigned int inst,
			const Constraint_ptr& con)
	{
		_instToConst[inst]->push_back(con);
	}

	unsigned int size() const {
		return _instToConst.size();
	}

	ConstraintsList& forInstance(size_t i) {
		return *_instToConst[i];
	}

	const ConstraintsList& forInstance(size_t i) const {
		return *_instToConst[i];
	}

	void freeze() {
		_frozen = true;
		calcNConstraints();
		unsigned int nextOffset = _offset;
		for (size_t i=0; i<_instToConst.size(); ++i) {
			_instOffsets[i] = nextOffset;
			nextOffset += _instToConst[i]->size();
		}
	}

	unsigned int nConstraints() {
		assert(_frozen);
		return _nConstraints;
	}

	void fromLambdas(const std::vector<double>& lambdas) {
		assert(_frozen);
		std::vector<double>::const_iterator lambdaIt =
			lambdas.begin() + _offset;
		typename InstanceToConstraintsList::iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			ConstraintsList& lst = **it;
			typename ConstraintsList::iterator conIt = lst.begin();
			for (; conIt!=lst.end(); ++conIt) {
				(*conIt)->setWeight(*lambdaIt);
				++lambdaIt;
			}
		}
	}

	void fromLambdas(unsigned int inst, 
			const std::vector<double>& lambdas) 
	{
		assert(_frozen);
		unsigned int offset = _instOffsets[inst];
		std::vector<double>::const_iterator lambdaIt =
			lambdas.begin() + offset;

		ConstraintsList& lst = *_instToConst[inst];
		for (typename ConstraintsList::iterator conIt = lst.begin();
				conIt!=lst.end(); ++conIt)
		{
			(*conIt)->setWeight(*lambdaIt);
			++lambdaIt;
		}
	}

	void zero() {
		typename InstanceToConstraintsList::iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			ConstraintsList& lst = **it;
			typename ConstraintsList::iterator conIt = lst.begin();
			for (; conIt!=lst.end(); ++conIt) {
				(*conIt)->setWeight(0.0);
			}
		}
	}

	void zero(unsigned int inst) {
		ConstraintsList& lst = *_instToConst[inst];
		for (typename ConstraintsList::iterator conIt = lst.begin();
				conIt!=lst.end(); ++conIt) 
		{
			(*conIt)->setWeight(0.0);
		}
	}

	void clearBoundsAndExpectations() {
		typename InstanceToConstraintsList::const_iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			ConstraintsList& lst = **it;
			typename ConstraintsList::iterator conIt = lst.begin();
			for (; conIt!=lst.end(); ++conIt) {
				(*conIt)->clearBoundAndExpectation();
			}
		}
	}

	void modifyFactors(GraphType& g) const {
		const ConstraintsList& lst = *_instToConst[g.id];
		for (typename ConstraintsList::const_iterator conIt = lst.begin();
				conIt!=lst.end(); ++conIt)
		{
			(*conIt)->modifyFactors(g);
		}
	}

	void dumpFactorModifications(const GraphType& g, std::wostream& out) const {
		const ConstraintsList& lst = *_instToConst[g.id];
		for (typename ConstraintsList::const_iterator conIt = lst.begin();
				conIt!=lst.end(); ++conIt)
		{
			(*conIt)->dumpFactorModifications(g, out);
		}
	}

	void updateBoundAndExpectation(GraphType& g) {
		ConstraintsList& lst = *_instToConst[g.id];
		for (typename ConstraintsList::iterator conIt = lst.begin();
				conIt!=lst.end(); ++conIt)
		{
			(*conIt)->updateBoundAndExpectation(g);
		}
	}

	double boundDotProduct(const std::vector<double>& lambdas) {
		assert(_frozen);
		std::vector<double>::const_iterator lambdaIt = 
			lambdas.begin() + _offset;

		double ret = 0.0;

		typename InstanceToConstraintsList::const_iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			ConstraintsList& lst = **it;
			typename ConstraintsList::const_iterator conIt = lst.begin();
			for (; conIt!=lst.end(); ++conIt) {
				ret += (*conIt)->bound() * (*lambdaIt);
				++lambdaIt;
			}
		}

		return ret;
	}

	void writeNegatedGradient(std::vector<double>& gradient) {
		assert(_frozen);
		std::vector<double>::iterator gradIt =
			gradient.begin() + _offset;

		typename InstanceToConstraintsList::const_iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			ConstraintsList& lst = **it;
			typename ConstraintsList::const_iterator conIt = lst.begin();
			for (; conIt!=lst.end(); ++conIt) {
				(*gradIt) = -(*conIt)->gradient();
				++gradIt;
			}
		}
	}

	const std::wstring& constraintName(unsigned int idx) const {
		return _constraintNames[idx];
	}

	void addConstraintName(const std::wstring& name) {
		_constraintNames.push_back(name);
	}


private:
	typedef std::vector<ConstraintsList_ptr> InstanceToConstraintsList;
	InstanceToConstraintsList _instToConst;
	std::vector<unsigned int> _instOffsets;
	bool _frozen;
	unsigned int _nConstraints;
	unsigned int _offset;
	std::vector<std::wstring> _constraintNames;

	void calcNConstraints() {
		_nConstraints = 0;
		typename InstanceToConstraintsList::const_iterator it =
			_instToConst.begin();
		for (; it!=_instToConst.end(); ++it) {
			_nConstraints += (*it)->size();
		}
	}

};

template <typename GraphType>
class ConstraintsType {
public:
	typedef std::vector<boost::shared_ptr<Constraint<GraphType > > > type;
	typedef boost::shared_ptr<type> ptr;
};

template <typename GraphType>
class InstanceConstraintsType {
public:
	typedef InstanceConstraintsCollection<GraphType> type;
	typedef boost::shared_ptr<type> ptr;
};

namespace DumpUtils {
	void dumpIntVector(const std::vector<unsigned int>& vec, std::wostream& out);

	template<class View>
	void dumpFeatureVector(const std::vector<unsigned int>& vec, std::wostream& out) {
		bool first = true;
		for (std::vector<unsigned int>::const_iterator it = vec.begin(); 
				it != vec.end(); ++it)
		{
			if (!first) {
				out << L", ";
			}
			first = false;
			out << View::FactorType::featureName(*it);
		}
	}
};

};

#endif

