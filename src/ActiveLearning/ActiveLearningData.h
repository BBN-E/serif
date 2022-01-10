#ifndef _ACTIVE_LEARNING_DATA_H_
#define _ACTIVE_LEARNING_DATA_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "ActiveLearning/AnnotatedInstanceRecord.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/FeatureGrid.h"


BSP_DECLARE(AnnotatedFeatureRecord)
BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(InstanceAnnotationDBView)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(InstanceStrategy)
BSP_DECLARE(DataView)

class ActiveLearningData {
public:
	/****************
	 * Types
	 ****************/
	typedef std::map<size_t, AnnotatedInstanceRecord> InstanceAnnotations;
	typedef std::map<size_t, int> InstanceHashToAnnotationMap;
	typedef std::vector<AnnotatedFeatureRecord_ptr> FeatureAnnotations;

	/****************
	 * Accessors
	 ****************/
	//LearnIt2Trainer_ptr trainer() const;
	const FeatureAnnotations& annotatedFeatures() const;
	
	bool featureBeingAnnotated(int idx) const;
	bool featureAlreadyAnnotated(int idx) const;
	const std::set<int>& featuresBeingAnnotated() const;
	
	// returns 1 for positive features, -1 for negative, 0 for unannotated
	int instanceAnnotation(int inst) const;
	
	bool instanceHasAnnotatedFeature(int inst) const;

	/************
	 * Modifiers
	 ************/
	// not sure about this one either
	void registerAnnotation(size_t feat, const std::string& state);
	void updateInstanceAnnotations(const InstanceHashToAnnotationMap& hashToAnnMap);
	void startAnnotation(int feat);
	void stopAnnotation(int feat);

	/***********
	 * Actions
	 ***********/
	void syncToDB();
	bool updateBetweenIterations();

	/*******************
	 * For testing only
	 *******************/
	void _test_clear_annotations();
private:
	ActiveLearningData(InstanceAnnotationDBView_ptr db, FeatureAlphabet_ptr alphabet,
		InstanceHashes_ptr _instance_hashes, DataView_ptr data);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ActiveLearningData, InstanceAnnotationDBView_ptr, 
		FeatureAlphabet_ptr, InstanceHashes_ptr, DataView_ptr );
	
	bool addOrUpdateInstanceAnnotation(AnnotatedInstanceRecord air);
	void addAnnotatedFeature(AnnotatedFeatureRecord_ptr afr);

	InstanceAnnotationDBView_ptr _db;
	FeatureAlphabet_ptr _alphabet;
	
	InstanceAnnotations _annotatedInstances;
	std::vector<int> _fastInstanceAnnotations;
	FeatureAnnotations _annotatedFeatures;
	std::vector<bool> _instanceHasAnnotatedFeature;
	
	// to avoid threading issues we don't immediately update the list of
	// annotated features and instances. Instead, we add them to these two lists
	// and then add them at a safe time between iterations
	std::vector<AnnotatedInstanceRecord> _newAnnotatedInstances;
	std::vector<AnnotatedFeatureRecord_ptr> _newAnnotatedFeatures;

	// these are the features which should be currently displayed in the
	// annotation interface
	std::set<int> _featuresBeingAnnotated;
	std::vector<int> _features_previously_annotated;

	// TODO: sort and describe the below
	static AnnotatedFeatureRecord_ptr makeAnnotation(int feat, const std::string& state);
	InstanceHashes_ptr _instance_hashes;

	DataView_ptr _data;
};
#endif
