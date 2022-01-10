#include "Generic/common/leak_detection.h"
#include "RandomStrategy.h"

#include "LearnIt/Eigen/Core"
// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/exceptions/CouldNotChooseFeatureException.h"

using namespace Eigen;

RandomStrategy::RandomStrategy(ActiveLearningData_ptr al_data, DataView_ptr data,
							   int tries) :
ActiveLearningStrategy(al_data), _data(data), _max_tries(tries)
{
}

int RandomStrategy::nextFeature() {
	for (int tries = 0; tries < _max_tries; ++tries) {
		int inst = rand() % _data->nInstances();
		int n_features = _data->features(inst).nonZeros();
		/*int n_slot_features = train->slotFeatures(inst).nonZeros();
		int n_sent_features = train->sentenceFeatures(inst).nonZeros();*/
		if (n_features == 0) {
//		if (n_slot_features == 0 && n_sent_features == 0) {
			continue;
		}
		//int feat_idx = rand() % (n_slot_features + n_sent_features);
		int feat_idx = rand() % n_features;
		//bool from_sent_features = feat_idx >= n_slot_features;
//		int feature_num_limit;
		// sorry for the ugly ternary - InnerIterator has no assignment
		// operator...
		/*SparseVector<double>::InnerIterator it(
			from_sent_features ? train->sentenceFeatures(inst)
								: train->slotFeatures(inst) );*/
		SparseVector<double>::InnerIterator it(_data->features(inst));
		
		/*if (from_sent_features) {
			feat_idx -= n_slot_features;
			feature_num_limit = n_sent_features;
		} else {
			feature_num_limit = n_slot_features;
		}*/

		int i =0;
		for (; i < n_features /*feature_num_limit*/ && it; ++i, ++it) {
			if (i == feat_idx) {
				int feat = it.index();
				// don't get duplicate features or those we have
				// annotated before
				if (!_alData->featureBeingAnnotated(feat) &&
					!_alData->featureAlreadyAnnotated(feat)) 
				{
					return feat;
				}
				break;
			}
		}
	}
	throw CouldNotChooseFeatureException("RandomStrategy::nextFeature",
		"Gave up after trying many times to find a feature "
		"not already selected");
}

void RandomStrategy::label(int k) {}
