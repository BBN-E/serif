#include "UnlabeledDisagreementLoss.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/SmoothedL1.h"

using Eigen::VectorXd;
using Eigen::SparseVector;

UnlabeledDisagreementLoss::UnlabeledDisagreementLoss(InferenceDataView_ptr view1, 
		InferenceDataView_ptr view2, ActiveLearningData_ptr alData,
		double whenAnnotatedLossWeight, double whenUnannotatedLossWeight,
		Loss lossFunc) 
: _data1(view1), _data2(view2), _alData(alData), 
_whenAnnotatedLossWeight(whenAnnotatedLossWeight),
_whenUnannotatedLossWeight(whenUnannotatedLossWeight),
_lossFunc(lossFunc) {}

double UnlabeledDisagreementLoss::operator()(VectorXd& gradient,
		size_t thread_idx, size_t n_threads) const {
	double loss = 0.0;
	double dbg = 0.0;
	for (unsigned int i=thread_idx; i<_data1->nInstances(); i+=n_threads) {
		if (!_alData->instanceAnnotation(i)) {
				// 1= slot
				// 2= sent
			SparseVector<double>::InnerIterator it1(_data1->features(i));
			SparseVector<double>::InnerIterator it2(_data2->features(i));

			// skip instances where one FV is empty
			if (it1 && it2) {
				// generally, we want to penalize disagreement more when
				// we have an annotation to make us more sure of one of
				// the models
				double lossWeight = _alData->instanceHasAnnotatedFeature(i)
					? _whenAnnotatedLossWeight : _whenUnannotatedLossWeight;

				// we dont take the prior into account when calculating disagreement,
				// since otherwise the default value for an instance we know nothing
				// about is rather extreme
				//double pos1 = _data1.predictionNoPrior(i);
				double pos1 = _data1->prediction(i);
				double neg1 = 1.0 - pos1;
				//double pos2= _data2.predictionNoPrior(i);
				double pos2= _data2->prediction(i);
				double neg2 = 1.0 - pos2;

				if (_lossFunc == SQUARED) {
					double diff = pos1 - pos2;
					loss -= lossWeight * (diff)*(diff);
					double both_mult = lossWeight * 2.0 * diff;
					double mult1 = both_mult*pos1*neg1;
					double mult2 = both_mult*pos2*neg2;
					for (; it1; ++it1) {
						gradient(it1.index()) -= mult1*it1.value();
						/*if (_debug_gradient) {
						_local_gradient(it1.index()) -= mult1*it1.value();
						}*/
					}
					for (; it2; ++it2) {
						gradient(it2.index()) += mult2*it2.value();
						/*if (_debug_gradient) {
						_local_gradient(it2.index()) += mult2*it2.value();
						}*/
					}
				} else if (_lossFunc == KL) {
					//_loss -= _lossWeight * (pos1*log(pos1/pos2) + neg1*log(neg1/neg2));
					loss -= lossWeight * (pos1*(log(pos1)-log(pos2)) 
						+ neg1*(log(neg1)-log(neg2)));
					//double factor1 = -_lossWeight * pos1*neg1*(log((pos1*neg2)/(neg1 * pos2)));
					double factor1 = -lossWeight * pos1*neg1*
						(log(pos1*neg2) -log(neg1 * pos2));
					double factor2 = -lossWeight * (neg1*pos2-pos1*neg2);
				
					for (; it1; ++it1) {
						gradient(it1.index()) += factor1 * it1.value();
					}
					for (; it2; ++it2) {
						gradient(it2.index()) += factor2 * it2.value();
					}
				} else if (_lossFunc == L1) {
					loss += SmoothedL1::SmoothedL1ProbProb(pos1, pos2, 
						_data1->features(i), _data2->features(i),
						gradient, lossWeight);
				}
			}
		}
	}
	_debug_loss = loss;
	return loss;
}

std::wstring UnlabeledDisagreementLoss::status() const {
	return L"diss_loss = " + boost::lexical_cast<std::wstring>(_debug_loss);
}

void UnlabeledDisagreementLoss::snapshot() {
}

double UnlabeledDisagreementLoss::forInstance(int inst, 
								Eigen::SparseVector<double>& gradient) const
{
	double ret = 0.0;

	if (!_alData->instanceAnnotation(inst)) {
		// 1= slot
		// 2= sent
		SparseVector<double>::InnerIterator it1(_data1->features(inst));
		SparseVector<double>::InnerIterator it2(_data2->features(inst));

		// skip instances where one FV is empty
		if (it1 && it2) {
			double lossWeight = _alData->instanceHasAnnotatedFeature(inst)
				? _whenAnnotatedLossWeight : _whenUnannotatedLossWeight;

			// we dont take the prior into account when calculating disagreement,
			// since otherwise the default value for an instance we know nothing
			// about is rather extreme
			//double pos1 = _data1.predictionNoPrior(i);
			double pos1 = _data1->prediction(inst);
			double neg1 = 1.0 - pos1;
			//double pos2= _data2.predictionNoPrior(i);
			double pos2= _data2->prediction(inst);
			double neg2 = 1.0 - pos2;

			double diff = pos1 - pos2;
			ret -= lossWeight * (diff)*(diff);
			double both_mult = lossWeight * 2.0 * diff;
			double mult1 = both_mult*pos1*neg1;
			double mult2 = both_mult*pos2*neg2;
			for (; it1; ++it1) {
				gradient.coeffRef(it1.index()) -= mult1*it1.value();
				
				/*if (_debug_gradient) {
				_local_gradient(it1.index()) -= mult1*it1.value();
				}*/
			}
			for (; it2; ++it2) {
				gradient.coeffRef(it2.index()) += mult2*it2.value();
				
				/*if (_debug_gradient) {
				_local_gradient(it2.index()) += mult2*it2.value();
				}*/
			}
		}
	}
	return ret;
}

