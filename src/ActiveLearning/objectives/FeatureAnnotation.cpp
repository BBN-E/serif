#include "FeatureAnnotation.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/SmoothedL1.h"

using Eigen::VectorXd;
using Eigen::SparseVector;
using boost::lexical_cast;

FeatureAnnotationLoss::FeatureAnnotationLoss(InferenceDataView_ptr data,
	ActiveLearningData_ptr alData, double lossWeight, Loss lossFunc) 
: _data(data), _alData(alData), _lossWeight(lossWeight), _lossFunc(lossFunc) {}

std::wstring FeatureAnnotationLoss::status() const {
	return L"feat_loss: " + lexical_cast<std::wstring>(_debug_loss);
}

void FeatureAnnotationLoss::snapshot() {
}

double FeatureAnnotationLoss::forInstance(int inst, SparseVector<double>& gradient) const {
	double ret = 0.0;
	BOOST_FOREACH(AnnotatedFeatureRecord_ptr ann_feature, _alData->annotatedFeatures()) {
		int idx = ann_feature->idx;

		if (_data->features(inst).coeff(idx) > 0) {
			double observed = _data->features(inst).coeff(idx) * _data->prediction(inst);
			bool violated = ((observed < ann_feature->expectation
				&& ann_feature->positive_annotation) ||
				(observed > ann_feature->expectation
				&& !ann_feature->positive_annotation));

			if (violated) {
				double diff = ann_feature->expectation - observed;
				/*double scale = ann_feature->expectation / observed
				-  (1.0 - ann_feature->expectation) 
				/ (1.0 - observed);
				std::cout << "idx: " << ann_feature->idx << "; diff: " << diff 
				<< "; scale: " << scale << std::endl;*/
				ret -= _lossWeight * diff * diff;
				double common_factors = _lossWeight * 2.0 * diff;

				SparseVector<double>::InnerIterator it(_data->features(inst));
				double factor = common_factors * 
					_data->prediction(inst)*(1.0-_data->prediction(inst))
					/ _data->frequency(idx);
				for (; it; ++it) {
					gradient.coeffRef(it.index()) += factor * it.value();
				}
			}
		}
	}
	return ret;
}

double FeatureAnnotationLoss::operator()(VectorXd& gradient, size_t thread_idx,
		size_t n_threads) const 
{
	// Calculate penalties for annotated features
	// see Druck, et. al. "j Annotation Effort using 
	// Generalized Expectation Criteria", UMass Tech Report UM-CS-2007-62
	
	double loss = 0.0;
	double dbg = 0.0;

	BOOST_FOREACH(AnnotatedFeatureRecord_ptr ann_feature, 
					_alData->annotatedFeatures())
	{
		int idx = ann_feature->idx;
		// first gather expectations by current model ("observed")
		double observed = 0.0;
		SparseVector<double>::InnerIterator instIt(_data->instancesWithFeature(idx));
		for (; instIt; ++instIt) {
			observed += _data->prediction(instIt.index());
		}

		// then calculate penalty
		observed /= _data->frequency(idx);
		bool violated = ((observed < ann_feature->expectation
									&& ann_feature->positive_annotation) ||
								(observed > ann_feature->expectation
									&& !ann_feature->positive_annotation));

		double scale = ann_feature->expectation / observed
										-  (1.0 - ann_feature->expectation) 
										/ (1.0 - observed);
		
		if (_lossFunc == KL) {
			if (violated) {
				if (_lossFunc == KL) {
					loss -= _data->frequency(idx)*_lossWeight *
						(ann_feature->expectation 
						* log(ann_feature->expectation/observed) +
						(1.0 - ann_feature->expectation)
						* log((1.0 - ann_feature->expectation)/(1.0-observed)));

					double common_factors = _lossWeight * scale;
					SparseVector<double>::InnerIterator instIt(_data->instancesWithFeature(idx));

					for (; instIt; ++instIt) {
						int inst = instIt.index();
						if (inst % n_threads == thread_idx) {
							double factor = common_factors * _data->prediction(inst)
								*(1.0-_data->prediction(inst));
							for (SparseVector<double>::InnerIterator it(_data->features(inst)); it; ++it) {
								gradient(it.index()) += factor*it.value();
							}
						}
					}
				}
			} 
		} else { // _lossFunc == L1
				const double epsilon = SmoothedL1::DEFAULT_EPSILON;
				double diff = ann_feature->expectation - observed;

				if (!violated) {
					diff = 0;
				}

				double localLoss = sqrt(diff * diff + epsilon);
				loss -= _lossWeight * localLoss;

				if (violated) {
					double commonCoeff = _lossWeight * diff/localLoss;

					SparseVector<double>::InnerIterator instIt(_data->instancesWithFeature(idx));

					for (; instIt; ++instIt) {
						int inst = instIt.index();
						if (inst % n_threads == thread_idx) {
							double coeff = commonCoeff * _data->prediction(inst)
								*(1.0-_data->prediction(inst));
							for (SparseVector<double>::InnerIterator it(_data->features(inst)); it; ++it) {
								gradient(it.index()) += coeff*it.value();
							}
						}
					}
				}
			}
		}

	// warning: hack
	// processing of instances is parallelized but of features is not
	// since LL is calculated by features for this objective, we only want
	// to return the LL once, so we do it in thread 0
	_debug_loss = loss;
	return (thread_idx == 0)?loss:0.0;
}

