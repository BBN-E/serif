#include "RegularizationLoss.h"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "Generic/common/UnexpectedInputException.h"

using Eigen::VectorXd;

RegularizationLoss::RegularizationLoss(const VectorXd& params, 
	const VectorXd& pos_regularization_weights,
	const VectorXd& neg_regularization_weights, double lossWeight)
: _params(params), _pos_regularization_weights(pos_regularization_weights),
_neg_regularization_weights(neg_regularization_weights),
_lossWeight(lossWeight) 
{
	if (_pos_regularization_weights.size() != _params.size()
			|| _neg_regularization_weights.size() != _params.size()) {
		throw UnexpectedInputException(
			"RegularizationLoss::RegularizationLoss",
			"Size of regularization weights vector must "
			"equal the number of features.");
	}
	for (int i=0; i<_pos_regularization_weights.size(); ++i) {
		if (_pos_regularization_weights[i] == 0.0
				|| _neg_regularization_weights[i] == 0.0)
		{
			std::wstringstream err;
			err << L"Feature " << i << L" has a regularization weight of 0.0;"
				<< L" your model will not converge properly.";
			throw UnexpectedInputException("RegularizationLoss::RegularizationLoss",
				err);
		}
	}
}

double RegularizationLoss::operator()(VectorXd& gradient, size_t thread_idx,
		size_t n_threads) const 
{
	double loss = 0.0;
	size_t sz = _params.size();
	
	for (size_t i=thread_idx; i<sz; i+=n_threads) {
		// regularization is weighted both overall and per-feature
		double weight = (_params[i]<0)?_neg_regularization_weights[i]:
			_pos_regularization_weights[i];

		loss -= weight* _lossWeight*_params[i]*_params[i];
		gradient[i] -= 2.0*weight* _lossWeight*_params[i];
	}

	_debug_loss = loss;
	return loss;
}

std::wstring RegularizationLoss::status() const {
	return L"reg_loss = " + boost::lexical_cast<std::wstring>(_debug_loss);
}

void RegularizationLoss::snapshot() {
}

double RegularizationLoss::forInstance(int inst, Eigen::SparseVector<double>& gradient) const {
	return 0.0;
}
