#pragma once
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/strategies/MaxScoreStrategy.h"

BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(DataView)
BSP_DECLARE(CoverageStrategy)

class CoverageStrategy : public MaxScoreStrategy {
public:
	virtual void label(int k);
	virtual void registerPop(int feat);
private:
	CoverageStrategy(ActiveLearningData_ptr al_data, DataView_ptr data,
		FeatureAlphabet_ptr alphabet);
	bool labelled(int feat) const;

	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(CoverageStrategy, ActiveLearningData_ptr,
		DataView_ptr, FeatureAlphabet_ptr); 
	void gather_feature_data();
	void load_frequency_modifiers();
	
	double score(int k);

	FeatureAlphabet_ptr _alphabet;
	DataView_ptr _data;

	Eigen::SparseVector<double> _feature_totals;
	Eigen::VectorXd _frequencies;
	Eigen::VectorXd _phi;
	std::vector<double> _n_instances;
	std::vector<double> _freq_modifiers;
	std::set<int> _labelled;
};
