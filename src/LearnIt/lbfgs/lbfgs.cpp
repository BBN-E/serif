#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#pragma warning(disable:4996) // disable Microsoft's warnings about boost

#include <iostream>

// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include <math.h>
#include "LearnIt/Eigen/Sparse"
#include "LearnIt/Eigen/Core"

#include "lbfgs.h"
#include "FunctionWithGradient.h"

using namespace Eigen;
using std::cerr; using std::endl; using std::cerr;

// the L-BFGS algorithm remembers the last several parameters and gradients
// in order to approximate the Hessian (second order partial derivatives)
// These are used to modify the gradient at the current point in order to
// get an update direction.  The Memory class holds that information.
class Memory {
public:
	Memory(int max_size, int n_features) : _max_size(max_size), _alpha(_max_size), 
		_gradientDiff(max_size, VectorXd::Zero(n_features)),
		_paramsDiff(max_size, VectorXd::Zero(n_features)),
		_actual_size(0), _head(0) {}
	
	int size() const { return _actual_size; }
	int max_size() const { return _max_size; }
	void pushState(const VectorXd& params, const VectorXd& gradient,
		double alpha)
	{
		// idx is the index to receive the new data
		// head is the index of the oldest entry
		int idx;
		if (_actual_size == 0) {
			// if we're currently empty, index and head
			// are the same
			++_actual_size;
			idx = _head = 0;
		} else if (_actual_size < _max_size) {
			// we're not yet full, so head doesn't need to move
			// we insert into the next empty spot
			idx = _head + _actual_size;
			++_actual_size;
		} else {
			// we're out of room, so the new data replaces 
			// the current head, and the head pointer is
			// shifted to the next oldest entry
			idx = _head;
			_head = (_head + 1) % _max_size;
		}
		_alpha[idx] = alpha;
		_gradientDiff[idx] = gradient;
		_paramsDiff[idx] = params;	
	}
	double alpha(int idx) {
		return _alpha[(_head + idx) % _max_size];
	}
	const VectorXd& gradientDiff(int idx) {
		return _gradientDiff[(_head + idx) % _max_size];
	}
	const VectorXd& paramsDiff(int idx) {
		return _paramsDiff[(_head + idx) % _max_size];
	}
	void clear() {
		_actual_size = 0;
	}
private:
	int _max_size;
	int _actual_size;
	std::vector<double> _alpha;
	std::vector<VectorXd > _gradientDiff;
	std::vector<VectorXd > _paramsDiff;
	std::vector<bool> _filled;
	int _head;
};

// do a line search on function func in the direction of the direction vector
// on termination, will return the size of the step taken in that direction,
// and the parameters (but not gradient!) will be updated for the new position 
// (that is, for their values *after* moving the selected distance along
// the direction vector)
// see Numerical Recipes in C section 9.7
double LBFGS::lineSearch(FunctionWithGradient& func, VectorXd& direction) 
{
	static const double MAX_STEP_LENGTH = 100.0;
	static const double ALPHA = 1e-4;
	size_t n_params = func.parameters().size();
	VectorXd original_parameters = func.parameters();
	VectorXd original_gradient = func.gradient();

	// ensure we don't step too far
	// This is actually unnecessary when used with L-BFGS because we always pass
	// a direction vector of unit length, but I put it in in case someone uses
	// this function for something else
//	double step_length = norm_2(direction);
	double step_length = direction.norm();
	if (step_length > MAX_STEP_LENGTH) {
		original_gradient *= MAX_STEP_LENGTH / step_length;
	}

//	double slope = inner_prod(original_gradient, direction);
	double slope = original_gradient.dot(direction);

	// check we're going up like we ought to be
	if (slope <= 0.0) {
		throw LineSearchSlopeNegative();
	}

	// calculate minimum step length
	double lambda_min_denom = 0.0;
	for (size_t i=0; i<n_params; ++i) {
		double temp = std::fabs(func.gradient()(i))/(std::max)(func.parameters()(i), 1.0);
		if (temp > lambda_min_denom) {
			lambda_min_denom = temp;
		}
	}
	double lambda_min = 1e-7 / lambda_min_denom;
	double lambda = 1.0;// lambda is our fraction of the full Newton step
	double previous_lambda = 0.0; // our previous estimate of lambda
	double previous_f = 0.0;
	double new_lambda = 1.0; // our new estimate of lambda
	double f_old = func.value();
	
	while (true) {
		// for the first iteration, old_lambda is zero, so we just update
		// our parameters by the full step size.  After than lamba will be 
		// shrinking, so lambda - old_lambda will be negative, so we will
		// be walking along the direction vector back towards our 
		// original parameters
		func.setParams(func.parameters() + (lambda - previous_lambda) * direction);
		
		if (lambda < lambda_min) {
			func.setParams(original_parameters);
			return 0.0;
		}

		double f = func.value();
		if (f >= f_old + lambda * slope * ALPHA) {
			// Yay, we increased the function enough.
			// 'enough' means the average slope (f-f_old)/lambda over
			// our step was at least ALPHA times the slope 
			// at our initial point
			return lambda;
		} else {
			if (lambda == 1.0) {
				// for our first backtrack, we use a quadratic approximation
				// (from Numerical Recipes 9.7.11)
				new_lambda = -slope / (2.0 * (f - f_old - slope));
			} else {
				// on the following iterations we use a cubic approximation
				// (Numericla Recipes 9.7.14)
				double rhs1 = f - f_old - lambda * slope;
				double rhs2 = previous_f - f_old - previous_lambda * slope;
				double a = (rhs1/(lambda*lambda) - rhs2/(previous_lambda*previous_lambda))
								/(lambda-previous_lambda);
				double b = (-previous_lambda*rhs1/(lambda*lambda)
							+lambda*rhs2/(previous_lambda*previous_lambda))
							/(lambda-previous_lambda);
				if (a == 0.0) {
					new_lambda = -slope / (2.0 * b);
				} else {
					double discriminant = b*b - 3.0*a*slope;
					if (discriminant < 0) {
						new_lambda = 0.5 * lambda;
					} else if (b <= 0.0) {
						new_lambda = (-b + sqrt(discriminant))/(3.0*a);
					} else {
						new_lambda = -slope / (b + sqrt(discriminant));
					}
					// decrease by at least half each iteration
					new_lambda = (std::min)(new_lambda, 0.5 * lambda);
				}
			}
			previous_lambda = lambda;
			previous_f = f;
			// don't backtrack more than 10x in one iteration
			lambda = (std::max)(new_lambda, 0.1*lambda);
		}
	}
}

bool LBFGS::optimize(FunctionWithGradient& func, int max_iterations,
					 double gradient_tolerance, double objective_tolerance)
{
OUTER:
	while (true) {
		// these could be made user modifiable parameters
		const int MEMORY_SIZE = 4;
		size_t n_features = func.parameters().size();
		bool converged = false;

		// remember our starting place
		double old_f = func.value();
		VectorXd old_params = func.parameters();
		VectorXd old_gradient = func.gradient();

		// we need a normalized gradient for the line search
		double gradient_length = old_gradient.norm();
		if (std::fabs(gradient_length) <= gradient_tolerance) {
			// gradient is already zero, so we're already converged
			func.postIterationHook();
			return true;
		}
		VectorXd direction = old_gradient/gradient_length;

		// do an initial line search in the direction of the starting gradient
		// after this call, parameters (but not gradient!) will be updated for the new
		// position
		double step_size = lineSearch(func, direction);

		if (step_size == 0) {
			// converged
			func.postIterationHook();
			return true;
		}

		Memory memory(MEMORY_SIZE, static_cast<int>(n_features));

		VectorXd beta(memory.max_size());
		VectorXd gradient_diff(n_features);
		VectorXd params_diff(n_features);
		VectorXd gradient(n_features);
		VectorXd params(n_features);

		double new_val = 0.0;
		double old_val = func.value();
		std::cout << "foo" << std::endl;
		for (int iter = 0; !converged && iter < max_iterations; ++iter) {
			func.gradient(gradient);
			func.parameters(params);
			gradient_diff = gradient - old_gradient;
			params_diff = params - old_params;

			double alpha = gradient_diff.dot(params_diff);
			double sigma = gradient_diff.dot(gradient_diff);

			// L-BFGS uses the last MEMORY_SIZE steps to estimate the Hessian
			memory.pushState(params_diff, gradient_diff, alpha);

			// we start with the current gradient and modify it based on
			// the last MEMORY_SIZE steps
			func.gradient(direction);

			for (int m = memory.size() - 1; m >= 0; --m) {
				beta(m) = direction.dot(memory.paramsDiff(m))
					/memory.alpha(m);
				direction -= beta(m)*memory.gradientDiff(m);
			}
			direction *= alpha / sigma;
			for (int m = 0; m < memory.size(); ++m) {
				double zeta = memory.gradientDiff(m).dot(direction);
				direction += memory.paramsDiff(m) * (beta(m) - zeta / memory.alpha(m));
			}
			direction *= -1.0;
			old_params = params;
			old_gradient = gradient;

			try {
				step_size = lineSearch(func, direction);
			} catch (LineSearchSlopeNegative) {
				// if fancy LBFGS search fails, just take the gradient
				SessionLogger::warn("line_search_slope") 
					<< "LBFGS Line search slope negative";
				memory.clear();
				func.gradient(direction);
				direction = direction / direction.norm();
				step_size = lineSearch(func, direction);
			}
			gradient_length = func.gradient().norm();
			new_val = func.value();
			double obj_diff = new_val - old_val;
			old_val = new_val;
			if (gradient_length <= gradient_tolerance || step_size == 0
					|| fabs(obj_diff) < objective_tolerance) 
			{
				converged = true;
			}
			if (func.postIterationHook()) {
				// if this return true, we need to clear our memory and
				// restart. Otherwise we can end up with negative line
				// slopes and other problems.
				memory.clear();
				goto OUTER;
			}
		}
		return converged;
	}

	return true;
	//	return converged;
}

