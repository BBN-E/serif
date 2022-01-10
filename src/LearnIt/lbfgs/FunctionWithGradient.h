#ifndef _FUNCTION_WITH_GRADIENT_
#define _FUNCTION_WITH_GRADIENT_

// get rid of Visual Studio's bogus warnings about boost code
#pragma warning( push )
#pragma warning( disable : 4996 )

// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "LearnIt/Eigen/Core"

// To make a FunctionWithGradient, simply subclass from this class and
// override the virtual recalc method.
// 
// we borrow the design from Mallet of having the function track its own
// parameters. The reason for this is that it's often convenient to 
// calculate the gradient as we calculate the value and then cache both
// until the parameters change
class FunctionWithGradient {
public:
	FunctionWithGradient(int n_params);
	
	void parameters(Eigen::VectorXd& dest) const;
	const Eigen::VectorXd& parameters() const;
	size_t nParams() const;
	void setParam(unsigned int idx, double val);
	void setParams(const Eigen::VectorXd& newParams);
	void incrementParams(const Eigen::SparseVector<double>& inc, double eta = 1.0);
	void incrementParam(int param, double amount);
	
	double value() const;
	void gradient(Eigen::VectorXd& dest) const;
	const Eigen::VectorXd& gradient() const;
	virtual bool postIterationHook() = 0;
	void setDirty();

	// testing only
	void _test_clear_params();
protected:
	void updateIfNeeded() const;
	// given params as input, write to the other two arguments the value and
	// gradient of the function at that point
	virtual void recalc(const Eigen::VectorXd& params, 
						double& value, 
						Eigen::VectorXd& gradient) const = 0;
private:
	mutable bool _dirty;
	mutable Eigen::VectorXd _params;
	mutable Eigen::VectorXd _gradient;
	mutable double _value;
};
#pragma warning( pop )
#endif
