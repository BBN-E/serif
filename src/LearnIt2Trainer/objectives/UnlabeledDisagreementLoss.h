#ifndef _DISS_LOSS_H_
#define _DISS_LOSS_H_

#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

BSP_DECLARE(InferenceDataView)
BSP_DECLARE(ActiveLearningData)

class UnlabeledDisagreementLoss : public InstancewiseObjectiveFunctionComponent {
public:
	enum Loss { SQUARED, KL, L1};
	UnlabeledDisagreementLoss(InferenceDataView_ptr view1, 
		InferenceDataView_ptr view2, ActiveLearningData_ptr alData, 
		double whenAnnotatedLossWeight, double whenUnannotatedLossWeight,
		Loss lossFunc = KL);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx,
			size_t n_threads) const;
	std::wstring status() const;
	void snapshot();
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
private:
	InferenceDataView_ptr _data1;
	InferenceDataView_ptr _data2;
	ActiveLearningData_ptr _alData;
	double _whenAnnotatedLossWeight;
	double _whenUnannotatedLossWeight;
	mutable double _debug_loss;
	//bool _klDivergence;
	Loss _lossFunc;
};

#endif
