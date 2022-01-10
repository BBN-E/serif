#ifndef _INSTANCE_LOSS_H_
#define _INSTANCE_LOSS_H_

#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

BSP_DECLARE(InferenceDataView)
BSP_DECLARE(ActiveLearningData)

class InstanceLoss : public InstancewiseObjectiveFunctionComponent {
public:
	InstanceLoss(InferenceDataView_ptr data, ActiveLearningData_ptr alData,
		double lossWeight);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx, 
			size_t n_threads) const;
	std::wstring status() const;
	void snapshot();
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
private:
	InferenceDataView_ptr _data;
	ActiveLearningData_ptr _alData;
	double _lossWeight;
	mutable double _debug_loss;
};

#endif
