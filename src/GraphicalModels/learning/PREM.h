#ifndef _PR_EM_CLUSTERING_MODEL_
#define _PR_EM_CLUSTERING_MODEL_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/UnrecoverableException.h"
#include "EM.h"
#include "../../GraphicalModels/pr/Constraint.h"
#include "../../GraphicalModels/DataSet.h"
#include "../../GraphicalModels/Graph.h"
#include "../../GraphicalModels/Alphabet.h"
#include "LBFGSProjectionProblem.h"

namespace GraphicalModel {
template <class GraphType> 
class PREM : public EM<PREM<GraphType>, GraphType> {
public:
	enum LearningScheme { SGD, LBFGS };
	typedef typename ConstraintsType<GraphType>::type Constraints;
	typedef typename ConstraintsType<GraphType>::ptr Constraints_ptr;
	typedef Constraint<GraphType> InstanceConstraint;
	typedef typename boost::shared_ptr<Constraint<GraphType> >
		InstanceConstraint_ptr;
	typedef typename InstanceConstraintsCollection<GraphType>::ptr InstanceConstraints_ptr;

	PREM(const DataSet<GraphType>& data, unsigned int n_classes, 
			const Constraints_ptr& constraints, 
			const InstanceConstraints_ptr& instanceConstraints,
			LearningScheme scheme, bool verbose)
		: _constraints(constraints), _instanceConstraints(instanceConstraints),
		_scheme(scheme), _verbose(verbose) {}

	double eImpl(DataSet<GraphType>& data) {
		clearAllConstraints();
		clearAllFactorModifications(data);

		calcLambdas(data);

		// do final expectation using the calculated lambdas
		double LL = 0.0;
		for (typename DataSet<GraphType>::GraphVector::iterator gIt = data.graphs.begin();
				gIt!=data.graphs.end(); ++gIt)
		{
			(*gIt)->clearFactorModifications();
			modifyFactors(**gIt);

			LL+=(*gIt)->inference();
		}

		dumpLambdas();

		return LL;
	}

	void resetInstanceConstraints(const InstanceConstraints_ptr& instCons) {
		_instanceConstraints = instCons;
	}

private:
	void clearAllConstraints() {
		for (typename Constraints::iterator cIt = _constraints->begin(); 
				cIt!=_constraints->end(); ++cIt) 
		{
			(*cIt)->clearWeight();
		}

		_instanceConstraints->zero();
	}

	void clearAllFactorModifications(DataSet<GraphType>& data) {
		for (typename DataSet<GraphType>::GraphVector::iterator gIt = data.graphs.begin();
				gIt != data.graphs.end(); ++gIt)
		{
			(*gIt)->clearFactorModifications();
		}
	}

	void dumpLambdas() {
		std::stringstream debugMsg;

		debugMsg << L"Final lambdas: ";
		for (typename Constraints::iterator cIt = _constraints->begin();
				cIt!=_constraints->end(); ++cIt)
		{
			(*cIt)->dumpWeight(debugMsg);
			debugMsg << L"\t";
		}
		debugMsg << std::endl;
		debugMsg << L"Need to do something about dumping instance-level constraints here." 
			<< std::endl;
		SessionLogger::info("lambdas") << debugMsg.str();
	}

	void modifyFactors(GraphType& g) {
		for (typename Constraints::iterator cIt = _constraints->begin();
				cIt!=_constraints->end(); ++cIt)
		{
			(*cIt)->modifyFactors(g);
		}

		_instanceConstraints->modifyFactors(g);
	}

	void calcLambdas(DataSet<GraphType>& data) {
		if (_scheme == SGD) {
			sgdProject(data, 25);
		} else {
			lbfgsProject(data);
		}
	}

	void sgdProject(DataSet<GraphType>& data, unsigned int minibatch_size) {
		throw UnrecoverableException("PREM::sgdProject",
				"If you want to use SGD-based projection, you need to fix the "
				"PREM::sgdProject function to support instance-level constraints");
		// project expectations to space satisfying posterior
		// constraints via projected stoachastic gradient descent
		double LL = 0.0;
		double learning_rate = 1.0;
		const unsigned int NUM_SGD_ITERATIONS = 10;
		for (unsigned int it = 0; it<NUM_SGD_ITERATIONS; ++it) {
			for (unsigned int first = 0; first < data.graphs.size(); first+=minibatch_size) {
				for (typename Constraints::iterator cIt = _constraints->begin();
						cIt != _constraints->end(); ++cIt)
				{
					(*cIt)->clearBoundAndExpectation();
				}

				for (unsigned int inst = first; 
						inst < first + minibatch_size && inst < data.graphs.size();
						++inst) 
				{
					GraphType& graph = *data.graphs[inst];
					graph.clearFactorModifications();
					for (typename Constraints::iterator cIt = _constraints->begin();
							cIt!=_constraints->end(); ++cIt)
					{
						(*cIt)->modifyFactors(graph);
					}
					graph.inference();
					for (typename Constraints::iterator cIt = _constraints->begin();
							cIt!=_constraints->end(); ++cIt)
					{
						(*cIt)->updateBoundAndExpectation(graph);
					}
				}

				for (typename Constraints::iterator cIt = _constraints->begin();
						cIt!=_constraints->end(); ++cIt)
				{
					(*cIt)->updateByProjectedGradient(learning_rate);
				}
			}
			learning_rate *= 0.8;
		}
	}

	void lbfgsProject(DataSet<GraphType>& data) {
		boost::shared_ptr<LBFGS::LBFGSOptimizableFunction> problem = 
			boost::make_shared<LBFGSProjectionProblem<GraphType> >(&data, 
					_constraints, _instanceConstraints);
		LBFGS::BoundedLBFGSOptimizer optimizer(problem, LBFGS::NonNegativeBounds(),
				LBFGS::BoundedLBFGSOptimizer::LOW_ACCURACY, 1.0e-5, 3);
		optimizer.optimize(40, _verbose);
	}


protected:
	Constraints_ptr _constraints;
	InstanceConstraints_ptr _instanceConstraints;
	LearningScheme _scheme;
	bool _verbose;
};
};

#endif

