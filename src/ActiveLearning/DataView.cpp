#include "DataView.h"
#include "Generic/common/leak_detection.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#pragma warning(push, 0)
#include <boost/thread.hpp>
#pragma warning(pop)
#include "Generic/common/ParamReader.h"

using Eigen::VectorXd;
using Eigen::SparseVector;
using std::set;

const double InferenceDataView::_epsilon = .000000001;

DataView::DataView(unsigned int n_instances, unsigned int n_features)
: _features(n_instances, SparseVector<double>(n_features)),
_features_to_instances(n_features, SparseVector<double>(n_instances)),
_nInstances(n_instances), _nFeatures(n_features),
_frequencies(VectorXd::Zero(n_features))
{
	
}

unsigned int DataView::nInstances() const {
	return _nInstances;
}

unsigned int DataView::nFeatures() const {
	return _nFeatures;
}

void DataView::observeFeaturesForInstance(unsigned int inst, 
										  const set<unsigned int>& features) {
	BOOST_FOREACH(unsigned int feat, features) {
		_features[inst].insert(feat) = 1.0;
		++_frequencies[feat];
		_features_to_instances[feat].insert(inst) = 1.0;
	}
	_features[inst].finalize();
}

void DataView::finishedLoading() {
	BOOST_FOREACH(Eigen::SparseVector<double>& vec, _features_to_instances) {
		vec.finalize();
	}
}

const SparseVector<double>& DataView::features(int inst) const {
	return _features[inst];
}

const SparseVector<double>& DataView::instancesWithFeature(int feat) const {
	return _features_to_instances[feat];
}

double DataView::frequency(int feat) const {
	return _frequencies[feat];
}

bool DataView::hasFV(int inst) const {
	return features(inst).nonZeros() > 0;
}

InferenceDataView::InferenceDataView(int n_instances, int n_features) 
: DataView(n_instances, n_features), _predictions(VectorXd::Zero(n_instances)),
	_threads(ParamReader::getOptionalIntParamWithDefaultValue("inference_threads", 1))
{}

void InferenceDataView::inferenceWork(const VectorXd& params,
		int num_threads, int thread_idx) const
{
	for (unsigned int i=thread_idx; i< nInstances(); i+=num_threads) {
		double score = features(i).dot(params);

		_predictions[i] = 1.0/(1.0 + exp(-score));
		if (_predictions[i] == 1.0) {
			_predictions[i] = 1.0 - _epsilon;
		} else if (_predictions[i] == 0) {
			_predictions[i] = _epsilon;
		}
	}
}

void InferenceDataView::inference(const VectorXd& params) const {
	if (_threads == 1) {
		inferenceWork(params, 1, 0);
	} else {
		typedef boost::shared_ptr<boost::thread> Thread_ptr;
		std::vector<Thread_ptr > threads;
		for (int i=0; i<_threads; ++i) {
			threads.push_back(Thread_ptr(new boost::thread(
				&InferenceDataView::inferenceWork, this, params, _threads, i)));
		}
		BOOST_FOREACH(Thread_ptr thread, threads) {
			thread->join();
		}
	}
}

double InferenceDataView::prediction(int inst) const {
	return _predictions[inst];
}

void InferenceDataView::inference(int inst, const VectorXd& params) const {
	double score = features(inst).dot(params);

	_predictions[inst] = 1.0/(1.0 + exp(-score));
}
