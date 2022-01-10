#include "InstanceStrategy.h"

#include <vector>
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "ActiveLearning/DataView.h"

using Eigen::SparseVector;

InstanceStrategy::InstanceStrategy(DataView_ptr data) :
_data(data)  {}

std::vector<int> 
InstanceStrategy::clientInstances(int feat, size_t max_instances) const {
	//const SparseVector<int>& instances_with_feature = _features_to_instances[feat];
	std::vector<int> client_instances;
	//LearnIt2Trainer_ptr trn = trainer();

	const SparseVector<double>& instances_with_feature = 
		_data->instancesWithFeature(feat);

	if ((size_t)instances_with_feature.nonZeros() <= max_instances) {
		for (SparseVector<double>::InnerIterator it(instances_with_feature); it; ++it) {
			client_instances.push_back(it.index());
		}	
	} else {
		while (client_instances.size() < (size_t)max_instances) { 
			int available_instances = instances_with_feature.nonZeros();
			SparseVector<double>::InnerIterator it(instances_with_feature);
			int roll = rand() % available_instances;

			for (int i=0; i<roll; ++i) ++it;
			int choice = it.index();
			if (find(client_instances.begin(), client_instances.end(), choice) 
				== client_instances.end()) 
			{
				client_instances.push_back(choice);
			}
		}
	}

	return client_instances;
}
