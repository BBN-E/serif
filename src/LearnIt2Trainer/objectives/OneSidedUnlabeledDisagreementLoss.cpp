#include "Generic/common/leak_detection.h"

#include "OneSidedUnlabeledDisagreementLoss.h"
#include <iostream>
#include <limits>
#include <boost/lexical_cast.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/ActiveLearningData.h"

using Eigen::VectorXd;
using Eigen::SparseVector;

OneSidedUnlabeledDisagreementLoss::OneSidedUnlabeledDisagreementLoss(
	InferenceDataView_ptr optData, InferenceDataView_ptr referenceData, 
	ActiveLearningData_ptr alData, double whenAnnotatedLossWeight, 
	double whenUnannotatedLossWeight) 
: _optimizeData(optData), _referenceData(referenceData), _alData(alData), 
_whenAnnotatedLossWeight(whenAnnotatedLossWeight),
_whenUnannotatedLossWeight(whenUnannotatedLossWeight)
{}

double OneSidedUnlabeledDisagreementLoss::operator()(VectorXd& gradient,
		size_t thread_idx, size_t n_threads) const 
{
	double loss = 0.0;
	for (unsigned int i=thread_idx; i<_optimizeData->nInstances(); i+=n_threads) {
		if (!_alData->instanceAnnotation(i)) {
				// 1= slot
				// 2= sent
			SparseVector<double>::InnerIterator optIt(_optimizeData->features(i));
			SparseVector<double>::InnerIterator refIt(_referenceData->features(i));

			// skip instances where one FV is empty
			if (optIt && refIt) {
				// generally, we want to penalize disagreement more when
				// we have an annotation to make us more sure of one of
				// the models
				double lossWeight = _alData->instanceHasAnnotatedFeature(i)
					? _whenAnnotatedLossWeight : _whenUnannotatedLossWeight;

				double posOpt = _optimizeData->prediction(i);
				double negOpt = 1.0 - posOpt;
				double posRef= _referenceData->prediction(i);
				double negRef = 1.0 - posRef;

				loss -= lossWeight * (posRef*(log(posRef)-log(posOpt)) 
					+ negRef*(log(negRef)-log(negOpt)));
				
				double factor = -lossWeight * (posOpt - posRef);

				for (; optIt; ++optIt) {
					gradient(optIt.index()) += factor * optIt.value();
				}
			}
		}
	}
	_debug_loss = loss;
	return loss;
}

std::wstring OneSidedUnlabeledDisagreementLoss::status() const {
	return L"diss_loss = " + boost::lexical_cast<std::wstring>(_debug_loss);
}

void OneSidedUnlabeledDisagreementLoss::snapshot() {
}

double OneSidedUnlabeledDisagreementLoss::forInstance(int inst, 
													  Eigen::SparseVector<double>& gradient) const
{
	double ret = 0.0;
	int i = inst;
	if (!_alData->instanceAnnotation(i)) {
		// 1= slot
		// 2= sent
		SparseVector<double>::InnerIterator optIt(_optimizeData->features(i));
		SparseVector<double>::InnerIterator refIt(_referenceData->features(i));

		// skip instances where one FV is empty
		if (optIt && refIt) {
			// generally, we want to penalize disagreement more when
			// we have an annotation to make us more sure of one of
			// the models
			double lossWeight = _alData->instanceHasAnnotatedFeature(i)
				? _whenAnnotatedLossWeight : _whenUnannotatedLossWeight;

			double posOpt = _optimizeData->prediction(i);
			double negOpt = 1.0 - posOpt;
			double posRef= _referenceData->prediction(i);
			double negRef = 1.0 - posRef;

			ret = -lossWeight * (posRef*(log(posRef)-log(posOpt)) 
				+ negRef*(log(negRef)-log(negOpt)));

			double factor = -lossWeight * (posOpt - posRef);

			for (; optIt; ++optIt) {
				gradient.coeffRef(optIt.index()) += factor * optIt.value();
			}
		}
	}
	return ret;
}

