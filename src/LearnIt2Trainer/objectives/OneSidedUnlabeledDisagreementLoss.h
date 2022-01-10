#ifndef _ONE_SIDED_DISS_LOSS_H_
#define _ONE_SIDED_DISS_LOSS_H_

#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

BSP_DECLARE(InferenceDataView)
BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(OneSidedUnlabeledDisagreementLoss)

class OneSidedUnlabeledDisagreementLoss : public InstancewiseObjectiveFunctionComponent {
public:
	OneSidedUnlabeledDisagreementLoss(InferenceDataView_ptr view1, 
		InferenceDataView_ptr view2, ActiveLearningData_ptr alData, 
		double whenAnnotatedLossWeight, double whenUnannotatedLossWeight);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx,
			size_t n_threads) const;
	std::wstring status() const;
	void snapshot();
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
private:
	InferenceDataView_ptr _optimizeData;
	InferenceDataView_ptr _referenceData;
	ActiveLearningData_ptr _alData;
	double _whenAnnotatedLossWeight;
	double _whenUnannotatedLossWeight;
	mutable double _debug_loss;
	//bool _klDivergence;
};

#endif
