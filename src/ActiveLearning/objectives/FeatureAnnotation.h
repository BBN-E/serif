#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

BSP_DECLARE(InferenceDataView);
BSP_DECLARE(ActiveLearningData);

class FeatureAnnotationLoss : public InstancewiseObjectiveFunctionComponent {
public:
	enum Loss {KL, L1};
	FeatureAnnotationLoss(InferenceDataView_ptr data, ActiveLearningData_ptr alData,
		double lossWeight, Loss loss = KL);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx,
			size_t n_threads) const;
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
	std::wstring status() const;
	void snapshot();
private:
	const InferenceDataView_ptr _data;
	const ActiveLearningData_ptr _alData;
	const double _lossWeight;
	mutable double _debug_loss;
	Loss _lossFunc;
};
