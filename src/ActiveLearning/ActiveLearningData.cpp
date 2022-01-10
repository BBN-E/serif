#include "Generic/common/leak_detection.h"
#include "ActiveLearningData.h"

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "ActiveLearning/AnnotatedInstanceRecord.h"
#include "ActiveLearning/InstanceAnnotationDB.h"
#include "ActiveLearning/FeatureGrid.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "LearnIt/Eigen/Sparse"
#include "LearnIt/db/LearnItDB.h"
//#include "exceptions/ExpiredTrainerException.h"
//#include "trainer.h"

using std::string; using std::wstring;
using std::make_pair;
using std::vector;
using boost::make_shared;
using Eigen::SparseVector;

ActiveLearningData::ActiveLearningData(InstanceAnnotationDBView_ptr db, 
			FeatureAlphabet_ptr alphabet, const InstanceHashes_ptr instance_hashes, 
			DataView_ptr data) :
_db(db), _alphabet(alphabet), _instance_hashes(instance_hashes),
_fastInstanceAnnotations(instance_hashes->size(), AnnotatedInstanceRecord::NO_ANNOTATION),
_instanceHasAnnotatedFeature(instance_hashes->size(), false),
_data(data)
{
	BOOST_FOREACH(AnnotatedInstanceRecord air, db->getInstanceAnnotations()) {
		try {
			addOrUpdateInstanceAnnotation(air);
		} catch (UnknownInstanceHashException& e) {
			SessionLogger::warn("active_learning") << "No instance matches hash " 
				<< e.hash << " stored for an annotation; ignoring"; 
		}
	}

	BOOST_FOREACH(AnnotatedFeatureRecord_ptr afr, alphabet->annotations()) {
		addAnnotatedFeature(afr);
	}
}

bool ActiveLearningData::addOrUpdateInstanceAnnotation(AnnotatedInstanceRecord air) {
	bool updatedInstances = false;
	_fastInstanceAnnotations[_instance_hashes->instance(air.instance_hash())] =
		air.annotation();

	// check if we already have an annotation for this instance
	InstanceAnnotations::iterator probe = 
		_annotatedInstances.find(air.instance_hash());
	SessionLogger::info("register_annotation") << 
		L"Registered annotation for hash " << air.instance_hash();
	if (probe == _annotatedInstances.end()) {
		// if not, add it if it's not a neutral annotation
		// (no need to add neutrals, since everything is neutral
		// by default)
		if (air.annotation()) {
			_annotatedInstances.insert(make_pair(air.instance_hash(), air));
			updatedInstances = true;
		}
	} else {
		if (probe->second.annotation() != air.annotation()) {
			probe->second.setAnnotation(air.annotation());
			updatedInstances = true;
		}
	}

	_fastInstanceAnnotations[_instance_hashes->instance(air.instance_hash())] =
		air.annotation();

	return updatedInstances;
}

int ActiveLearningData::instanceAnnotation(int inst) const {
	return _fastInstanceAnnotations[inst];
}

void ActiveLearningData::startAnnotation(int feat) {
	_featuresBeingAnnotated.insert(feat);
}

void ActiveLearningData::stopAnnotation(int feat) {
	_featuresBeingAnnotated.erase(feat);
}

bool ActiveLearningData::featureBeingAnnotated(int idx) const {
	return std::find(_featuresBeingAnnotated.begin(), _featuresBeingAnnotated.end(),
					idx) != _featuresBeingAnnotated.end();
}

bool ActiveLearningData::featureAlreadyAnnotated(int idx) const {
	return std::find(_features_previously_annotated.begin(), _features_previously_annotated.end(),
					idx) != _features_previously_annotated.end();
}

const std::vector<AnnotatedFeatureRecord_ptr>& ActiveLearningData::annotatedFeatures() const {
	return _annotatedFeatures;
}

AnnotatedFeatureRecord_ptr ActiveLearningData::makeAnnotation(int feat, 
													const string& state)
{
	if ("positive" == state) {
		return make_shared<AnnotatedFeatureRecord>(feat, true, 0.85);
	} else if ("weakpositive" == state) {
		return make_shared<AnnotatedFeatureRecord>(feat, true, 0.30);
	} else if ("negative" == state) {
		return make_shared<AnnotatedFeatureRecord>(feat, false, 0.025);
	} else if ("weaknegative" == state) {
		return make_shared<AnnotatedFeatureRecord>(feat, false, 0.05);
	} 

	throw UnexpectedInputException("LearnIt2Trainer::makeAnnotation", 
		(string("Unknown annotation type ") + state).c_str());
}

void ActiveLearningData::updateInstanceAnnotations(const InstanceHashToAnnotationMap&
											   hashToAnnMap)
{
	BOOST_FOREACH_PAIR(size_t hash, int annotation, hashToAnnMap) {
		_newAnnotatedInstances.push_back(AnnotatedInstanceRecord(hash, annotation));
	}
}

void ActiveLearningData::registerAnnotation(size_t feat, const string& state) {
	_features_previously_annotated.push_back(static_cast<int>(feat));

	if (state != "ignore") {
		_newAnnotatedFeatures.push_back(makeAnnotation(static_cast<int>(feat), state));
	}
}


void ActiveLearningData::syncToDB() {
	_alphabet->recordAnnotations(_annotatedFeatures);
	std::vector<AnnotatedInstanceRecord> airs;
	BOOST_FOREACH_PAIR(size_t hash, const AnnotatedInstanceRecord air, _annotatedInstances) {
		airs.push_back(air);
	}
	_db->recordAnnotations(airs);
}

const std::set<int>& ActiveLearningData::featuresBeingAnnotated() const {
	return _featuresBeingAnnotated;
}

bool ActiveLearningData::updateBetweenIterations() {
	bool ret = false;
	if (!_newAnnotatedFeatures.empty()) {
		BOOST_FOREACH(const AnnotatedFeatureRecord_ptr annFeat, _newAnnotatedFeatures) {
			addAnnotatedFeature(annFeat);
		}
		_newAnnotatedFeatures.clear();
		ret = true;
	}
	if (!_newAnnotatedInstances.empty()) {
		bool updatedInstances = false;

		BOOST_FOREACH(const AnnotatedInstanceRecord annInst, _newAnnotatedInstances) {
			updatedInstances = 
				addOrUpdateInstanceAnnotation(annInst) || updatedInstances;
		}
		_newAnnotatedInstances.clear();
		if (updatedInstances) {
			ret = true;
		}
	}
	return ret;
}

void ActiveLearningData::addAnnotatedFeature(AnnotatedFeatureRecord_ptr afr) {
	_annotatedFeatures.push_back(afr);
	for (SparseVector<double>::InnerIterator inst_it(_data->instancesWithFeature(afr->idx)); 
		inst_it; ++inst_it) 
	{
		_instanceHasAnnotatedFeature[inst_it.index()] = true;
	}
}

void ActiveLearningData::_test_clear_annotations() {
	_annotatedFeatures.clear();
	std::fill(_instanceHasAnnotatedFeature.begin(),
		_instanceHasAnnotatedFeature.end(), false);
}

bool ActiveLearningData::instanceHasAnnotatedFeature(int inst) const {
	return _instanceHasAnnotatedFeature[inst];
}
