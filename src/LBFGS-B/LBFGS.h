#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

namespace LBFGS {

	class BoundsBase {
		public:
			virtual double upperBound(size_t i) const = 0;
			virtual double lowerBound(size_t i) const = 0;
			virtual bool hasUpperBound(size_t i) const = 0;
			virtual bool hasLowerBound(size_t i) const = 0;
	};

	class Unbounded : public BoundsBase {
		public:
			double upperBound(size_t i) const { return 0.0;}
			double lowerBound(size_t i) const { return 0.0;}
			bool hasUpperBound(size_t i) const { return false;}
			bool hasLowerBound(size_t i) const { return false;}
	};

	class NonNegativeBounds : public BoundsBase {
		public:
			double upperBound(size_t i) const { return 0.0;}
			double lowerBound(size_t i) const { return 0.0;}
			bool hasUpperBound(size_t i) const { return false;}
			bool hasLowerBound(size_t i) const { return true;}
	};

	class Bounds : public BoundsBase {
	public:
		Bounds(size_t sz) : _hasUpperBound(sz, false), _hasLowerBound(sz,false),
		_upperBounds(sz), _lowerBounds(sz) {}

		bool hasUpperBound(size_t i) const {
			return _hasUpperBound[i];
		}

		bool hasLowerBound(size_t i) const {
			return _hasLowerBound[i];
		}

		double upperBound(size_t i) const {
			return _upperBounds[i];
		}

		double lowerBound(size_t i) const {
			return _lowerBounds[i];
		}
	protected:
		std::vector<double> _upperBounds;
		std::vector<double> _lowerBounds;
		std::vector<bool> _hasLowerBound;
		std::vector<bool> _hasUpperBound;
	};

	class ModifiableBounds : public Bounds {
		public:
			ModifiableBounds(size_t sz) : Bounds(sz) {}

			void setUpperBound(size_t i, double val) {
				_upperBounds[i] = val;
				_hasUpperBound[i] = true;
			}

			void setLowerBound(size_t i, double val) {
				_lowerBounds[i] = val;
				_hasLowerBound[i] = true;
			}
	};


	BSP_DECLARE(LBFGSOptimizableFunction)
	class LBFGSOptimizableFunction {
	public:
		virtual double recompute() = 0;
		virtual void reset() = 0;
		virtual size_t nParams() const = 0;
		virtual std::vector<double>& gradient() = 0;
		virtual std::vector<double>& params() = 0;
		virtual void dumpGradientAndParameters() const = 0;
		virtual void dumpParameters() const = 0;
		virtual void postIteration(unsigned int iteration) const {}
	};

	class RestartLBFGS{
	public:
		RestartLBFGS() {}
	};

class BoundedLBFGSOptimizer {
public:
	BoundedLBFGSOptimizer(const LBFGSOptimizableFunction_ptr& func, 
			const BoundsBase& bounds, double objective_tolerance = MODERATE_ACCURACY,
			double projected_gradient_tolerance = 1.0e-5,int memory=5)
		: _memory(memory), _func(func),_task(60),_iprint(-1), 
		_objective_tolerance(objective_tolerance),
	_projected_gradient_tolerance(projected_gradient_tolerance), _lsave(4),
	_isave(44), _dsave(29), _csave(60), _u(func->nParams()), _l(func->nParams()),
	_nbd(func->nParams()), _iwa(3*func->nParams()), 
	_wa(2*_memory*func->nParams() + 5*func->nParams() + 11*_memory*_memory
			+ 8*_memory)
	{
		for (size_t i=0; i<func->nParams(); ++i) {
			bool has_upper_bound = bounds.hasUpperBound(i);
			bool has_lower_bound = bounds.hasLowerBound(i);

			if (has_upper_bound) {
				_u[i] = bounds.upperBound(i);
			}
			if (has_lower_bound) {
				_l[i] = bounds.lowerBound(i);
			}

			if (has_upper_bound && has_lower_bound) {
				_nbd[i] = 2;
			} else if (has_upper_bound) {
				_nbd[i] = 3;
			} else if (has_lower_bound) {
				_nbd[i] = 1;
			} else {
				_nbd[i] = 0;
			}
		}
	}	
	void optimize(unsigned int max_iterations = 100, bool verbose=false);
	void dump(const std::string& msg, bool print_g) const;
private:
	// keep _memory first because others depend on it for initialization
	int _memory;
	LBFGSOptimizableFunction_ptr _func;
	int _iprint;
	double _f;
	double _objective_tolerance, _projected_gradient_tolerance;
	std::vector<char> _task,_csave;
	std::vector<int> _lsave, _isave;
	std::vector<double> _dsave;
	// sotre lwoer and upper bounds
	std::vector<double> _l, _u;
	// store what bounds if any are present
	std::vector<int> _nbd;

	// working memory for FORTRAN code
	std::vector<int> _iwa;
	std::vector<double> _wa;

	void startCommand();
	bool functionAndGradientRequested();
	bool anotherIterationRequested();
	bool converged();

public:
	// allows control of convergence tolerance
	// Higher accuracies are slower.
	// For most natural language applications, low accuracy is fine
	static const double LOW_ACCURACY;
	static const double MODERATE_ACCURACY;
	static const double HIGH_ACCURACY;
};
};


