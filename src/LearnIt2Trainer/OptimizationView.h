#ifndef _OPTIMIZATION_VIEW_H_
#define _OPTIMIZATION_VIEW_H_

#include <string>
#include <vector>
#include <time.h>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "LearnIt/lbfgs/FunctionWithGradient.h"

BSP_DECLARE(OptimizationView)
BSP_DECLARE(InstancewiseObjectiveFunctionComponent)
BSP_DECLARE(LearnIt2Trainer)
BSP_DECLARE(InferenceDataView)
BSP_DECLARE(FeatureAlphabet)

class OptimizationView : public FunctionWithGradient {
public:
	void addObjectiveComponent(InstancewiseObjectiveFunctionComponent_ptr objComp);
	virtual bool postIterationHook();
	void optimize(int max_view_iterations, double lbfgs_gradient_tolerance,
			double lbfgs_objective_tolerance);
	InferenceDataView_ptr data();

	std::wstring UIDiagnosticString() const;
	void betweenIterations();
	bool ownsFeature(unsigned int i) const;
	void fixFeature(unsigned int feat, double value);
	void inference() const;

	void _test_clear_objective_components();

	OptimizationView(InferenceDataView_ptr data, FeatureAlphabet_ptr alphabet,
			unsigned int threads);
private:
	void recalc(const Eigen::VectorXd& params, double& value,
		Eigen::VectorXd& gradient) const;
	void inference(const Eigen::VectorXd& params) const;
	void applyObjectiveComponents(size_t thread_idx) const;

	InferenceDataView_ptr _data;
	std::vector<InstancewiseObjectiveFunctionComponent_ptr> _objectiveComponents;
	mutable double _ll;
	double _debug_ll;
	unsigned int _iteration;
	double _regLoss;
	double _debug_reg_loss;
	
	bool _sgd;
	double _sgd_q, _debug_e;
	mutable double _grad_norm;
	Eigen::VectorXi _lastRegularized;
	mutable std::vector<Eigen::VectorXd> _threadGradients;
	mutable std::vector<double> _thread_lls;
	size_t _n_threads;

	time_t _old_time;

	std::vector<bool> _ownedFeatures;
	std::vector<unsigned int> _fixedFeatures;

	FeatureAlphabet_ptr _debug_alphabet;

	void initializeOwnedFeatures();
	void setApplicableParams(const Eigen::VectorXd& weights);

	void printLargestGradientFeatures();
};

#endif
