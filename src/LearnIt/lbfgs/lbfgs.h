//
// Implementation of the Limited-Memory  Broyden–Fletcher–Goldfarb–Shanno
// optimization algorithm (L-BFGS)
//
// The canonical citation seems to be
// Byrd, Richard H.; Lu, Peihuang; Nocedal, Jorge; Zhu, Ciyou (1995). 
// "A Limited Memory Algorithm for Bound Constrained Optimization". 
// SIAM Journal on Scientific and Statistical Computing 16 (5): 1190–1208. 

// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "LearnIt/Eigen/Core"

class FunctionWithGradient;

class LBFGS {
public:
	// initial search position should be the current parameters of func
	// upon completion of optimization, optimal position will be in the
	// same place
	static bool optimize(FunctionWithGradient& func, int max_iterations = 100,
		double gradient_tolerance = 1e-4, double objective_tolerance = 0.0);

	// searches along the vector direction from the current parameters of func
	// for a parameter setting which yields a higher value than the starting
	// value.
	// See lnsrch in Numerical Recipes in C
	static double lineSearch(FunctionWithGradient& func, Eigen::VectorXd& direction);
};

// exception thrown if the direction of the line search is wrong
class LineSearchSlopeNegative : public std::exception {
};

