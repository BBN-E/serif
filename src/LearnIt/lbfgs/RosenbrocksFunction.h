#include <boost/numeric/ublas/vector.hpp>
#include "FunctionWithGradient.h"

// Rosenbrock's 'banana' function is a commonly used test function for L-BFGS
// see http://en.wikipedia.org/wiki/Rosenbrock_function
class RosenbrocksFunction : public FunctionWithGradient {
public:
	RosenbrocksFunction() : 
	  FunctionWithGradient(2) {}
	bool postIterationHook();
private:
	virtual void recalc(const Eigen::VectorXd& params, double& value, 
		Eigen::VectorXd& gradient) const;
	mutable double _lastVal;
};

class NegativeRosenbrocksFunction : public FunctionWithGradient {
public:
	NegativeRosenbrocksFunction() : FunctionWithGradient(2) {}
	bool postIterationHook();
private:
	virtual void recalc(const Eigen::VectorXd& params, double& value, 
		Eigen::VectorXd& gradient) const;
	mutable double _lastVal;
};

