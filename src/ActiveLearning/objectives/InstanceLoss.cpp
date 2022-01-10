#include "InstanceLoss.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/ActiveLearningData.h"

using Eigen::VectorXd;

InstanceLoss::InstanceLoss(InferenceDataView_ptr data, ActiveLearningData_ptr alData,
						   double lossWeight) 
: _data(data), _alData(alData), _lossWeight(lossWeight) {}

double InstanceLoss::operator()(VectorXd& gradient, size_t thread_idx, 
		size_t n_threads) const 
{
	double loss = 0.0;
	for (int i=static_cast<int>(thread_idx); i<static_cast<int>(_data->nInstances()); i+=static_cast<int>(n_threads)) {
		// ann will be 1 if positive, -1 if negative, 0 if unannotated
		if (int ann=_alData->instanceAnnotation(i)) {
			bool pos_example = (ann == 1);

			if (_data->hasFV(i)) {
				double pred = pos_example ? _data->prediction(i) 
					: (1.0 - _data->prediction(i));
				loss += _lossWeight * log(pred);
				if (pos_example) {
					gradient += _lossWeight *
						(1.0 - _data->prediction(i)) * _data->features(i);
				} else {	
					gradient -= _lossWeight * (_data->prediction(i)*_data->features(i));
				}
			}
		}
	}
	_debug_loss = loss;
	return loss;
}

double InstanceLoss::forInstance(int inst, Eigen::SparseVector<double>& gradient) const {
	double ret = 0.0;
	if (int ann=_alData->instanceAnnotation(inst)) {
		bool pos_example = (ann == 1);

		if (_data->hasFV(inst)) {
			double pred = pos_example ? _data->prediction(inst) 
				: (1.0 - _data->prediction(inst));
			ret += _lossWeight * log(pred);
			if (pos_example) {
				gradient += _lossWeight *
					(1.0 - _data->prediction(inst)) * _data->features(inst);
			} else {	
				gradient -= _lossWeight * (_data->prediction(inst)*_data->features(inst));
			}
		}
	}
	return ret;
}

std::wstring InstanceLoss::status() const {
	return L"inst_loss = " + boost::lexical_cast<std::wstring>(_debug_loss);
}

void InstanceLoss::snapshot() {
}
