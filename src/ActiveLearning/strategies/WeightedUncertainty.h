#ifndef _WEIGHTED_UNCERTAINTY_H_
#define _WEIGHTED_UNCERTAINTY_H_
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/strategies/MaxScoreStrategy.h"

BSP_DECLARE(ActiveLearningData);
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(InferenceDataView)
BSP_DECLARE(WeightedUncertainty)

class WeightedUncertainty : public MaxScoreStrategy {
public:
	WeightedUncertainty(ActiveLearningData_ptr al_data, InferenceDataView_ptr data,
		FeatureAlphabet_ptr alphabet);
	virtual void label(int k);
	virtual void modelUpdate();
private:
	/*BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(WeightedUncertainty, ActiveLearningData_ptr,
		DataView_ptr, FeatureAlphabet_ptr); */
	void gather_feature_data();
	void load_frequency_modifiers();
	void gather_frequencies(); 
	void gather_entropies();
	bool labelled(int feat) const;
	
	double score(int k);

	FeatureAlphabet_ptr _alphabet;
	InferenceDataView_ptr _data;

	Eigen::VectorXd _log_frequencies;
	Eigen::VectorXd _entropy;
	std::vector<double> _freq_modifiers;
	std::set<int> _labelled;
};
#endif
