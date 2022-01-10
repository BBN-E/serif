#ifndef _PRIOR_LOSS_H_
#define _PRIOR_LOSS_H_

#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

BSP_DECLARE(InferenceDataView)

class PriorLoss : public InstancewiseObjectiveFunctionComponent {
public:
	enum Loss {KL, L1};
	PriorLoss(InferenceDataView_ptr data, double prior, double lossWeight,
		Loss loss = KL);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx,
			size_t n_threads) const;
	std::wstring status() const;
	void snapshot();
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
	
private:
	const InferenceDataView_ptr _data;
	double _prior;
	double _lossWeight;
	mutable double _debug_loss;
	Loss _lossFunc;
};

#endif
