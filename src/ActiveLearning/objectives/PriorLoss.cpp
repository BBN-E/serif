#include "Generic/common/leak_detection.h"

#include "PriorLoss.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/SmoothedL1.h"


using Eigen::VectorXd;
using Eigen::SparseVector;

PriorLoss::PriorLoss(InferenceDataView_ptr data, double prior, double lossWeight,
					 Loss loss) 
: _data(data), _prior(prior), _lossWeight(lossWeight), _lossFunc(loss) {}

double PriorLoss::operator()(VectorXd& gradient, size_t thread_idx,
		size_t n_threads) const 
{
	double loss = 0.0;

	for (int i=static_cast<int>(thread_idx); i<static_cast<int>(_data->nInstances()); i+=static_cast<int>(n_threads)) {
		double pos = _data->prediction(i);

		if (_lossFunc == KL) {
			double neg = 1.0 - pos;

			loss -= _lossWeight * (_prior * log (_prior/pos) +
				(1.0-_prior)*log((1.0-_prior)/neg));

			double factor = _lossWeight * (pos - _prior);

			SparseVector<double>::InnerIterator it(_data->features(i));
			for (; it; ++it) {
				int inst = it.index();
				gradient(it.index()) -= factor * it.value();
			}
		} else { // _lossFunc == L1
			loss += SmoothedL1::SmoothedL1ProbConstant(pos, _prior, 
				_data->features(i), gradient, _lossWeight);
		}
	} 
	_debug_loss = loss;
	return loss;
}

double PriorLoss::forInstance(int inst, SparseVector<double>& gradient) const {
	double ret = 0.0;

	double pos = _data->prediction(inst);

	if (_lossFunc == KL) {
		double neg = 1.0 - pos;

		ret -= _lossWeight * (_prior * log (_prior/pos) +
			(1.0-_prior)*log((1.0-_prior)/neg));

		double factor = _lossWeight * (pos - _prior);

		SparseVector<double>::InnerIterator it(_data->features(inst));
		for (; it; ++it) {
			int inst = it.index();
			gradient.coeffRef(it.index()) -= factor * it.value();
		}
	} else { // _lossFunc == L1
		throw UnrecoverableException("PriorLoss::forInstance",
			"Instance-wise loss not yet implemented for L1 (but it's not "
			"hard to do)");
/*		ret += SmoothedL1::SmoothedL1ProbConstant(pos, _prior,
			_data->features(inst), gradient, _lossWeight);*/
	}

	return ret;
}

std::wstring PriorLoss::status() const {
	return L"prior_loss = " + boost::lexical_cast<std::wstring>(_debug_loss);
}

void PriorLoss::snapshot() {
}
