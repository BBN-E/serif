#include "LearnIt/Eigen/Core"
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"

// approximates L1 loss by sqrt(x^2+e)
// several people use this trick, we should dig up the original citation if
// there is one
class SmoothedL1 {
public:
	static const double DEFAULT_EPSILON;
	// both of these return the loss and update the gradient
	static double SmoothedL1ProbConstant(double prob, double constant,
		const Eigen::SparseVector<double>& featureVector,
		Eigen::VectorXd& gradient, double weight = 1.0,
		double epsilon = DEFAULT_EPSILON);
	static double SmoothedL1ProbProb(double alpha, double beta,
		const Eigen::SparseVector<double>& alphaFV,
		const Eigen::SparseVector<double>& betaFV,
		Eigen::VectorXd& gradient, double weight = 1.0,
		double epsilon = DEFAULT_EPSILON);
};
