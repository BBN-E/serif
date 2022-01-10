#ifndef _REGULARIZATION_LOSS_H_
#define _REGULARIZATION_LOSS_H_

#include "LearnIt/Eigen/Core"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"

class DataView;

class RegularizationLoss : public InstancewiseObjectiveFunctionComponent {
public:
	RegularizationLoss(const Eigen::VectorXd& params, 
		const Eigen::VectorXd& pos_regularization_weights, 
		const Eigen::VectorXd& neg_regularization_weights, 
		double lossWeight);
	double operator()(Eigen::VectorXd& gradient, size_t thread_idx,
			size_t n_threads) const;
	std::wstring status() const;
	void snapshot();
	double forInstance(int inst, Eigen::SparseVector<double>& gradient) const;
private:
	const Eigen::VectorXd& _params;
	const Eigen::VectorXd _pos_regularization_weights;
	const Eigen::VectorXd _neg_regularization_weights;
	double _lossWeight;
	mutable double _debug_loss;
};

#endif
