#ifndef _DATA_VIEW_H_
#define _DATA_VIEW_H_

#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"

BSP_DECLARE(DataView)
BSP_DECLARE(InferenceDataView)

class DataView {
public:
	DataView(unsigned int n_instances, unsigned int n_features);
	
	double frequency(int feature) const;
	const Eigen::SparseVector<double>& instancesWithFeature(int feat) const;
	bool hasFV(int inst) const;
	const Eigen::SparseVector<double>& features(int inst) const;
	void observeFeaturesForInstance(unsigned int inst, 
		const std::set<unsigned int>& features);
	void finishedLoading();
	unsigned int nFeatures() const;
	unsigned int nInstances() const;
private:
	std::vector<Eigen::SparseVector<double> > _features_to_instances;
	std::vector<Eigen::SparseVector<double> > _features;
	Eigen::VectorXd _frequencies;
	unsigned int _nInstances;
	unsigned int _nFeatures;
};

class InferenceDataView : public DataView {
public:
	InferenceDataView(int n_instances, int n_features);

	void inference(const Eigen::VectorXd& params) const;
	void inference(int inst, const Eigen::VectorXd& params) const;
	double prediction(int inst) const;
private:
	mutable Eigen::VectorXd _predictions;
	static const double _epsilon;
	int _threads;

	void inferenceWork(const Eigen::VectorXd& params, int num_threads, 
			int thread_idx) const;
};
#endif
