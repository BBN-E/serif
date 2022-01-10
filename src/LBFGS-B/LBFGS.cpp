#include "Generic/common/leak_detection.h"
#include "LBFGS.h"

#include <vector>
#include <iostream>
#include "Generic/common/SessionLogger.h"

using namespace LBFGS;

const double BoundedLBFGSOptimizer::LOW_ACCURACY = 1e+12;
const double BoundedLBFGSOptimizer::MODERATE_ACCURACY = 1e+7;
const double BoundedLBFGSOptimizer::HIGH_ACCURACY = 1e1;

extern "C" {
extern int setulb_(int *n, int *m, double *x, double *l, double *u, int *nbd, 
		double *f, double *g, double *factr, double *pgtol, double *wa, 
		int *iwa, char *task, int *iprint, char *csave, int *lsave, int *isave,
		double *dsave, int, int);
};

using std::vector;
	
void BoundedLBFGSOptimizer::optimize (unsigned int max_iterations, bool verbose) {
	int n = _func->nParams();

	_f = _func->recompute();
	startCommand();

	if (verbose) {
		dump("Initial state", true);
	}
	unsigned int iterations = 1;
	while (true) {
		setulb_(&n,&_memory,&_func->params()[0],&_l[0],&_u[0],&_nbd[0],&_f,
				&_func->gradient()[0],&_objective_tolerance,
				&_projected_gradient_tolerance,&_wa[0],
				&_iwa[0],&_task[0],&_iprint,&_csave[0],
				&_lsave[0],&_isave[0],&_dsave[0],60,60);

		if (functionAndGradientRequested()) {
			_f = _func->recompute();
			dump("Line search", false);
		} else if (anotherIterationRequested()) {
			if (iterations < max_iterations) {
				if (verbose) {
					SessionLogger::info("lbfgsb") << "\tCompleted iteration " << iterations
						<< "; " << _isave[36] << " evaluations this iteration; "
						<< _isave[34] << " in total; " << _isave[38] << " free variables"
						<< " and " << _isave[39] << " active constraints.";
				}
				dump("State after iteration", true);
				try {
					_func->postIteration(iterations);
				} catch (RestartLBFGS&) {
					iterations = 0;
					startCommand();
					SessionLogger::warn("restart_lbfgs") << "Restart of LBFGS optimization requested";
					_func->reset();
				}

				++iterations;
				// do nothing; will automatically do another iteration
			} else {
				if (verbose) {
					dump("Reached maximum number of iterations", true);
				}
				break;
			}
		} else if (converged()) {
			if (verbose) {
				SessionLogger::info("lbfgsb") << "\tConverged on iteration " << iterations;
				dump("State at convergence", true);
			}
			break;
		} else {
			if (verbose) {
				SessionLogger::info("lbfgsb") << "\tAbnormal termination.";
			}
			break;
		}
	}
}

void BoundedLBFGSOptimizer::dump(const std::string& msg, bool print_g) const {
	if (print_g) {
		_func->dumpGradientAndParameters();
	} else {
		_func->dumpParameters();
	}
}

void BoundedLBFGSOptimizer::startCommand() {
	strcpy(&_task[0], "START");
	for (int i=5; i<60; ++i) {
		_task[i] = ' ';
	}
}

bool BoundedLBFGSOptimizer::functionAndGradientRequested() {
	return _task[0] == 'F' && _task[1] == 'G';
}

bool BoundedLBFGSOptimizer::anotherIterationRequested() {
	return _task[0] == 'N' && _task[1] == 'E' && _task[2] == 'W'
				&& _task[3] == '_' && _task[4] == 'X';
}

bool BoundedLBFGSOptimizer::converged() {
	return _task[0] == 'C' && _task[1] == 'O' && _task[2] == 'N'
				&& _task[3] == 'V';
}

/*
 // uncomment the following for testing
	
int main(int argc, const char** argv) {
	int sz = 25;
	RosenbrockFunc* func = new RosenbrockFunc(sz);
	ModifiableBounds bounds(sz);

	for (int i=0; i<func->nParams(); ++i) {
		func->params()[i] = 3.0;
	}

	// set bounds on (FORTRAN) odd-numbered variables
	// even for C++ since FORTRAN is 1-indexed
	for (int i=0; i<func->nParams(); i+=2) {
		bounds.setLowerBound(i, 1.0);
		bounds.setUpperBound(i, 100.0);
	}

	// set bounds on (FORTRAN) even-numbered variables
	// odd for C++ since FORTRAN is 1-indexed
	for (int i=1; i<func->nParams(); i+=2) {
		bounds.setLowerBound(i, -100.0);
		bounds.setUpperBound(i, 100.0);
	}

	BoundedLBFGSOptimizer optimizer(func, bounds,
			BoundedLBFGSOptimizer::MODERATE_ACCURACY, 1.0e-5);
	optimizer.optimize();
}

double square(double x) {
	return x*x;
}

class RosenbrockFunc : public Func {
	public:
		RosenbrockFunc(size_t n) : _x(n), _g(n)
	{ }

		double recompute() {
			double f = 0.25*square(_x[0]-1.0);
			size_t n = _x.size();

			for (int i=1; i<n; i++) {
				f = f + square((_x[i]-_x[i-1]*_x[i-1]));
			}
			f = 4.0*f;

			// compute gradient
			double t1 = _x[1]-_x[0]*_x[0];
			_g[0] = 2.0*(_x[0]-1.0)-16*_x[0]*t1;
			for (int i=1; i<n-1; ++i) {
				double t2 = t1;
				t1 = _x[i+1]-_x[i]*_x[i];
				_g[i]= 8.0*t2-16*_x[i]*t1;
			}
			_g[n-1] = 8.0*t1;
			return f;
		}

		std::vector<double>& params() {
			return _x;
		}

		std::vector<double>& gradient() {
			return _g;
		}

		size_t nParams() const {
			return _x.size();
		}


	private:
		std::vector<double> _x;
		std::vector<double> _g;
};*/


