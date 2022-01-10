#ifndef _LBFGS_PROJECTION_PROBLEM_H_
#define _LBFGS_PROJECTION_PROBLEM_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include "../../LBFGS-B/LBFGS.h"
#include "../DataSet.h"
#include "../pr/Constraint.h"

namespace GraphicalModel {

template <typename GraphType>
class LBFGSProjectionProblem : public LBFGS::LBFGSOptimizableFunction {
public:
	typedef typename ConstraintsType<GraphType>::type Constraints;
	typedef typename ConstraintsType<GraphType>::ptr Constraints_ptr;
	typedef typename boost::shared_ptr<Constraint<GraphType> >
		InstanceConstraint_ptr;
	typedef typename InstanceConstraintsCollection<GraphType>::ptr InstanceConstraints_ptr;

	LBFGSProjectionProblem(DataSet<GraphType> * data, 
			const Constraints_ptr& constraints, 
			const InstanceConstraints_ptr& instanceConstraints)
		: _constraints(constraints), _instanceConstraints(instanceConstraints),
		_lambdas(nParams(), 0.0), _gradient(nParams(), 0.0), _data(data)
	{ }

	// on destruction, we write our optimized values back to the constraints
	~LBFGSProjectionProblem() {
		writeAllLambdasBackToConstraints();
	}

	size_t nParams() const { 
		return _constraints->size() + _instanceConstraints->nConstraints();
	}

	std::vector<double>& gradient() {
		return _gradient;
	}

	std::vector<double>& params() {
		return _lambdas;
	}

	void reset() {
		BOOST_FOREACH(double& l, _lambdas) {
			l=0.0;
		}
	}

	void postIteration(unsigned int iteration) const {
		writeAllLambdasBackToConstraints();
		for (unsigned int inst = 0; inst< _data->graphs.size(); ++inst) {
			GraphType& graph = *_data->graphs[inst];
			graph.clearFactorModifications();
			modifyFactors(graph);
		}
		SessionLogger::info("foo") << L"Checking for conflicting constraints...";
		bool restart = false;
		for (unsigned int i=0; i<_data->graphs.size(); ++i) {
			const GraphType& graph = *_data->graphs[i];
			//if ((*_constraints)[0]->debugViolation(graph) > 0.1) {
			/*if (i == 109) {
				std::wstringstream ss;
				graph.dumpGraph();
				dumpFactorModifications(graph, ss);
				SessionLogger::info("constraints_for_graph")
					<< ss.str();
			}*/

			std::vector<Constraint<GraphType>* > constraintsToRelax;
			graph.findConflictingConstraints(constraintsToRelax);
			
			if (!constraintsToRelax.empty()) {
				restart = true;

				std::wstringstream badConstraints;
				badConstraints << L"Iteration " << iteration
					<< L" " << graph.id << L" seems to have a conflict. "
					<< L"Relaxing constraints ";
				BOOST_FOREACH(const Constraint<GraphType>* c, constraintsToRelax) {
					badConstraints << c->name() << L"\t";
				}
				SessionLogger::err("constraint_conflict") << badConstraints.str();

				graph.dumpGraph();

				std::wstringstream ss;
				dumpFactorModifications(graph, ss);
				SessionLogger::info("constraints_for_graph")
					<< ss.str();
			}

			BOOST_FOREACH(Constraint<GraphType>* c, constraintsToRelax) {
				c->relax(graph);
			}
		}

		if (restart) {
			throw LBFGS::RestartLBFGS();
		}
	}

	void dumpFactorModifications(const GraphType& g, std::wostream& out) const {
		for (typename Constraints::iterator cIt = _constraints->begin();
				cIt!=_constraints->end(); ++cIt) 
		{
			(*cIt)->dumpFactorModifications(g, out);
		}
		_instanceConstraints->dumpFactorModifications(g, out);
	}

	void dumpGradientAndParameters() const {
		std::stringstream msg;
		msg << "F = " << (_log_z_component + _bl_component) << "; Log(Z): "
			<< _log_z_component << "; bl: " << _bl_component << "\n";
		size_t name_width = 1;
		for (size_t i=0; i<_constraints->size(); ++i) {
			const Constraint<GraphType>& constraint = *(*_constraints)[i];
			name_width=(std::max)(name_width, constraint.name().size());
		}
		name_width += 10;

		msg << std::setw(name_width) << "Name" 
			<< std::setw(20) << "Bound" << std::setw(20)
			<< "Expectation" << std::setw(20) << "Gradient" << std::setw(20)
			<< "Parameter\n";
		for (size_t i=0; i< _constraints->size(); ++i) {
			const Constraint<GraphType>& constraint = *(*_constraints)[i];
			msg << std::setw(name_width) << constraint.name() 
				<< std::setw(20) << constraint.bound()
				<< std::setw(20) << constraint.expectation()
				<< std::setw(20) << constraint.gradient() 
				<< std::setw(20) << _lambdas[i]
				<< "\n";
		}

		SessionLogger::info("lbfgsb") << msg.str();
		SessionLogger::info("lbfgsb") << "Need to dump instance-level constraints as well.";
	}

	void dumpParameters() const {
		std::stringstream msg;
		msg << "F = " << (_log_z_component + _bl_component) << "; Log(Z): "
			<< _log_z_component << "; bl: " << _bl_component << "\n";
		msg << "X = [dumping of parameters temporarily disabled until we incorporate instance-level constraints]";
		/*msg << "X = [";
		for (size_t i=0; i<_lambdas.size(); ++i) {
			msg << _lambdas[i] << "\t";
		}
		msg << "]";*/
		SessionLogger::info("lbfgsb") << msg.str();
	}

	double recompute() {
		double objective = 0.0;
		// objective is:
		// -b*L -log Z(L)
		// where Z is partition function of projected distribution
		// see Posterior Regularization for Structured Latent Variable Models
		// Ganchev et al. 2010, Journal of Machine Learning Research

		// we compute log Z(L) first
		// doing the gradient second gives us more informative marginals
		// left over in the graphs for debugging purposes
		// inference is done with graphs not modified by constraints
		// because -log Z(l) is based on expectations relative to p, not q
		writeZerosToAllConstraints();
		_log_z_component = 0.0;
		for (unsigned int inst = 0; inst < _data->graphs.size(); ++inst) {
			GraphType& graph = *_data->graphs[inst];
			graph.clearFactorModifications();
			modifyFactors(graph);
			graph.inference();
			// this is hacky.  graph.logZ needs the lambda values on
			// the constraints, but inference here needs zeroes, so
			// we have to put the lambads back on and then zero them
			// out again as we process each graph
			writeSingleInstanceLambdasBackToConstraints(inst);
			// -log Z(l) part of objective
			_log_z_component -= graph.logZ(*_constraints, 
					_instanceConstraints->forInstance(inst));
			writeSingleInstanceZeroesToConstraints(inst);
			//objective-=graph.logZ(_constraints);
		}


		// now we compute the gradient...
		clearBoundsAndExpectations();
		writeAllLambdasBackToConstraints();

		for (unsigned int inst =0; inst < _data->graphs.size(); ++inst) {
			GraphType& graph = *_data->graphs[inst];
			graph.clearFactorModifications();

			modifyFactors(graph);
			graph.inference();
			updateBoundAndExpectations(graph);
		}

		writeGradient();

		// finally, compute the b*L component
		_bl_component = calcBoundsTimesLambdas();

		// switch sign because we're doing a maximization problem but
		// LBFGS-B wants to do minimization
		_log_z_component = -_log_z_component;
		_bl_component = -_bl_component;
		return _log_z_component + _bl_component;
		//return -objective;
	}

private:
	Constraints_ptr _constraints;
	InstanceConstraints_ptr _instanceConstraints;
	DataSet<GraphType> * _data;
	std::vector<double> _lambdas;
	std::vector<double> _gradient;
	double _bl_component;
	double _log_z_component;

	void writeCorpusLambdasBackToConstraints() const {
		for (size_t i=0; i<_constraints->size(); ++i) {
			(*_constraints)[i]->setWeight(_lambdas[i]);
		}
	}

	void writeAllLambdasBackToConstraints() const {
		writeCorpusLambdasBackToConstraints();
		_instanceConstraints->fromLambdas(_lambdas);
	}

	void writeSingleInstanceLambdasBackToConstraints(unsigned int inst) const {
		writeCorpusLambdasBackToConstraints();
		_instanceConstraints->fromLambdas(inst, _lambdas);
	}

	void writeZerosToCorpusConstraints() {
		for (size_t i=0; i<_constraints->size(); ++i) {
			(*_constraints)[i]->setWeight(0.0);
		}
	}

	void writeZerosToAllConstraints() {
		writeZerosToCorpusConstraints();

		_instanceConstraints->zero();
	}

	void writeSingleInstanceZeroesToConstraints(unsigned int inst) {
		writeZerosToCorpusConstraints();

		_instanceConstraints->zero(inst);
	}

	void clearBoundsAndExpectations() {
		for (typename Constraints::iterator cIt = _constraints->begin();
				cIt != _constraints->end(); ++cIt)
		{
			(*cIt)->clearBoundAndExpectation();
		}

		_instanceConstraints->clearBoundsAndExpectations();
	}

	void modifyFactors(GraphType& g) const {
		for (typename Constraints::iterator cIt = _constraints->begin();
				cIt!=_constraints->end(); ++cIt)
		{
			(*cIt)->modifyFactors(g);
		}
		_instanceConstraints->modifyFactors(g);
	}

	void updateBoundAndExpectations(GraphType& g) {
		for (typename Constraints::iterator cIt=_constraints->begin();
				cIt!=_constraints->end(); ++cIt)
		{
			(*cIt)->updateBoundAndExpectation(g);
		}

		_instanceConstraints->updateBoundAndExpectation(g);
	}

	double calcBoundsTimesLambdas() {
		double ret = 0.0;
		for (size_t i=0; i<_constraints->size(); ++i) {
			// -b*L part of objective
			//objective+=_constraints[i]->bound()*_lambdas[i];
			ret += (*_constraints)[i]->bound()*_lambdas[i];
		}
		ret += _instanceConstraints->boundDotProduct(_lambdas);
		return ret;
	}

	void writeGradient() {
		// corpus-level constraints
		for (size_t i=0; i<_constraints->size(); ++i) {
			_gradient[i] = -(*_constraints)[i]->gradient();
		}

		// instance-level constraints
		_instanceConstraints->writeNegatedGradient(_gradient);
	}


	};
};


#endif

