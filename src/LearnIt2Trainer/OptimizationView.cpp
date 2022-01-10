#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"
#include "OptimizationView.h"

#include "Generic/common/SessionLogger.h"
#include <vector>
#include <string>
#include <sstream>
#include <time.h>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/foreach_pair.hpp"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/lbfgs/lbfgs.h"
#include "LearnIt/Eigen/Core"
#include "LearnIt/Eigen/Sparse"

using namespace Eigen;

OptimizationView::OptimizationView(InferenceDataView_ptr data, 
   FeatureAlphabet_ptr alphabet, unsigned int threads) 
: FunctionWithGradient(alphabet->size()), _data(data),
_ll(0.0), _sgd(false), _sgd_q(0.0),
_lastRegularized(VectorXi::Zero(parameters().size())),
_ownedFeatures(alphabet->size(), false),
_debug_alphabet(alphabet),
_iteration(0), _old_time(0), _threadGradients(threads, VectorXd::Zero(parameters().size())),
_thread_lls(threads, 0.0), _n_threads(threads)
{
	initializeOwnedFeatures();
	setApplicableParams(alphabet->getWeights());
}

void OptimizationView::initializeOwnedFeatures() {
	for (unsigned int i=0; i<_data->nInstances(); ++i) {
		SparseVector<double>::InnerIterator it(_data->features(i));
		for (; it; ++it) {
			_ownedFeatures[it.index()] = true;
		}
	}
}

bool OptimizationView::ownsFeature(unsigned int i) const {
	return _ownedFeatures[i];
}

void OptimizationView::fixFeature(unsigned int i, double value) {
	setParam(i, value);
	_fixedFeatures.push_back(i);
}

void OptimizationView::setApplicableParams(const VectorXd& weights) {
	VectorXd applicableFeatures(weights.size());

	for (unsigned int i=0; i<_ownedFeatures.size(); ++i) {
		if (ownsFeature(i)) {
			applicableFeatures(i) = weights(i);
		} else {
			applicableFeatures(i) = 0.0;
		}
	}
	setParams(applicableFeatures);
}

void OptimizationView::addObjectiveComponent(
	InstancewiseObjectiveFunctionComponent_ptr objComp) 
{
	_objectiveComponents.push_back(objComp);
}

InferenceDataView_ptr OptimizationView::data() {
	return _data;
}

void OptimizationView::optimize(int max_view_iterations,
		double lbfgs_gradient_tolerance, double lbfgs_objective_tolerance) 
{
	int nInstances = _data->nInstances();
	_old_time = time(NULL);
	
	if (_sgd) {
		int max_epochs = 500;
		double sigma_squared = 144.0;
		SparseVector<double> gradient;
		std::vector<int> instanceOrder;
		
		double prev_ll = -numeric_limits<double>::infinity();
		for (int e = 0; e<max_epochs; ++e) {
			_ll = 0.0;
			instanceOrder.clear();

			for (unsigned int inst=0; inst<_data->nInstances(); ++inst) {
				instanceOrder.push_back(inst);
			}

			std::random_shuffle(instanceOrder.begin(), instanceOrder.end());
			double eta = 0.5 / (1.0 + e/100.0);
		
			BOOST_FOREACH(int inst, instanceOrder) {
				SparseVector<double>::InnerIterator it(_data->features(inst));
				for (; it; ++it) {
					int feat = it.index();
					incrementParam(feat, 
						((_lastRegularized(feat) - _sgd_q)/nInstances) * 
							parameters().coeff(feat)/sigma_squared);
					_lastRegularized(feat) = (int)_sgd_q;
				}
				_data->inference(inst, parameters());
				gradient.setZero();
				double instanceLoss = 0.0;
				BOOST_FOREACH(InstancewiseObjectiveFunctionComponent_ptr objComp, _objectiveComponents) {
					double foo = objComp->forInstance(inst, gradient);
				}
				incrementParams(gradient, eta);
			}
			++_sgd_q;
			_debug_e = e;
			_regLoss = 0.0;
			for (int i=0; i<parameters().size(); ++i) {
				_regLoss -= parameters().coeff(i)*parameters().coeff(i)
								/(2.0*sigma_squared);
			}

			_ll += _regLoss;

			if (_ll - prev_ll < 0.1) {
				// converged
				break;
			}
			prev_ll = _ll;
		}
	} else {
		LBFGS::optimize(*this, max_view_iterations, lbfgs_gradient_tolerance,
				lbfgs_objective_tolerance);
	}	
}

void OptimizationView::inference(/*const VectorXd& params*/) const {
	_data->inference(parameters());
}

void OptimizationView::inference(const VectorXd& params) const {
	_data->inference(params);
}

void OptimizationView::applyObjectiveComponents(size_t thread_idx) const {
	_thread_lls[thread_idx] = 0;
	double& ll = _thread_lls[thread_idx];
	VectorXd& gradient = _threadGradients[thread_idx];
	BOOST_FOREACH(ObjectiveFunctionComponent_ptr objComp, _objectiveComponents) {
		ll += (*objComp)(gradient, thread_idx, _n_threads);
	}
}

void OptimizationView::recalc(const VectorXd& params, double& value,
							  VectorXd& gradient) const
{
	if (_sgd) {
		value = _ll;
	}

	// calculate current model predicted probabilities for instances
	inference(params);

	BOOST_FOREACH(VectorXd& threadGradient, _threadGradients) {
		threadGradient.setZero(nParams());
	}

	for (size_t i=0; i< _n_threads; ++i) {
		_thread_lls[i] = 0.0;
	}

	typedef boost::shared_ptr<boost::thread> Thread_ptr;
	std::vector<Thread_ptr> threads;
	for (size_t i=0; i<_n_threads; ++i) {
		threads.push_back(Thread_ptr(new boost::thread(
				&OptimizationView::applyObjectiveComponents, this, i)));
	}

	BOOST_FOREACH(Thread_ptr thread, threads) {
		thread->join();
	}

	gradient.setZero(nParams());
	BOOST_FOREACH(VectorXd& threadGradient, _threadGradients) {
		gradient += threadGradient;
	}

	_ll = 0.0;
	for (size_t i=0; i<_n_threads; ++i) {
		_ll+= _thread_lls[i];
	}

	BOOST_FOREACH(int fixed_feature, _fixedFeatures) {
		gradient(fixed_feature) = 0.0;
	}

	if (_ll != _ll) {
		cout << "Blowup" << endl;
	}

	value = _ll;
	_grad_norm = gradient.norm();


}

wstring OptimizationView::UIDiagnosticString() const {
	wstringstream str;

	if (_sgd) {
		str << "epoch: " << _debug_e << "; q=" << _sgd_q << "; loss = " << (_debug_ll + _debug_reg_loss)
			<< "; reg_loss=" << _debug_reg_loss << "; param norm: " << parameters().norm();
	} else {
		str << L"iteration: " << _iteration << L": loss= " << _debug_ll << L"(";
		BOOST_FOREACH(ObjectiveFunctionComponent_ptr objComp, _objectiveComponents) {
			str << objComp->status() << L"; ";
		}
		str << L")";
	}

	return str.str();
}

bool OptimizationView::postIterationHook() {
	if (_iteration % 50 == 0) {
		time_t new_time = time(NULL);
		BOOST_FOREACH(const ObjectiveFunctionComponent_ptr obj, _objectiveComponents) {
			obj->snapshot();
		}
		SessionLogger::info("view_progress") << "LL: " << _ll 
			<< "\tgrad norm: " << _grad_norm << "; elapsed: " << (size_t)(new_time-_old_time);
		_old_time = new_time;
		/*if (_grad_norm < 1.0 && _ll > -23.0) {
			printLargestGradientFeatures();
		}*/
	}
	++_iteration;
	return false;
}

void OptimizationView::printLargestGradientFeatures() {
	std::vector<std::pair<double, std::pair<double, unsigned int> > > gradElements;
	for (int i=0; i<parameters().size(); ++i) {
		gradElements.push_back(make_pair(fabs(gradient()(i)), make_pair(gradient()(i), i)));
	}
	sort(gradElements.rbegin(), gradElements.rend());
	for (unsigned int i=0; i<10; ++i) {
		SessionLogger::info("foo") << L"\t" << _debug_alphabet->getFeatureName(gradElements[i].second.second) 
			<< L" = " << gradElements[i].second.first;
	}
}

void OptimizationView::_test_clear_objective_components() {
	_objectiveComponents.clear();
}

void OptimizationView::betweenIterations() {
	_ll = value();
	_debug_ll = _ll;
	BOOST_FOREACH(const ObjectiveFunctionComponent_ptr obj, _objectiveComponents) {
		obj->snapshot();
	}
}
