#include "Generic/common/leak_detection.h"
#include "LearnIt2TrainerUI.h"
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/foreach_pair.hpp"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/InstanceHashes.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/strategies/ActiveLearningStrategy.h"
#include "ActiveLearning/strategies/InstanceStrategy.h"
#include "ActiveLearning/ui/JSONUtils.h"
#include "ActiveLearning/ui/MongooseUtils.h"
#include "ActiveLearning/ui/ClientInstance.h"
#include "ActiveLearning/exceptions/CouldNotChooseFeatureException.h"
#include "trainer.h"

using std::string; using std::wstring; using std::stringstream;
using std::make_pair;
using boost::lexical_cast;
using boost::make_shared;
using boost::regex; using boost::match_results; using boost::regex_search;

class L2UIHook : public PostIterationCallback {
public:
	L2UIHook(LearnIt2TrainerUI* l2t) : _l2t(l2t) {}
	virtual bool operator()(); 
private:
	LearnIt2TrainerUI* _l2t;
};

bool L2UIHook::operator ()() {
	return _l2t->updateBetweenIterations();
}

LearnIt2TrainerUI::LearnIt2TrainerUI(LearnIt2Trainer_ptr trainer, 
									 ActiveLearningData_ptr data,
									 ActiveLearningStrategy_ptr strategy,
									 InstanceStrategy_ptr instanceStrategy,
									 InstanceHashes_ptr instanceHashes,
									 StringStore_ptr previewStrings) 
: _trainer(trainer), _alData(data), _strategy(strategy),
_finished(false), _instanceStrategy(instanceStrategy),
_preview_strings(previewStrings),
_instanceHashes(instanceHashes)
{
	trainer->registerPostIterationHook(make_shared<L2UIHook>(this));
	populateFeatureGrid();
}

bool LearnIt2TrainerUI::updateBetweenIterations() {
	return _alData->updateBetweenIterations();
}

void LearnIt2TrainerUI::populateFeatureGrid() {
	_featureGrid.clear();

	for (int i=0; i<25; ++i) {
		try {
			int rand_feature = _strategy->nextFeature();
			_featureGrid.push_back(rand_feature);
			_alData->startAnnotation(rand_feature);
		} catch (CouldNotChooseFeatureException) {
			break;
		}
	}
}

LearnIt2Trainer_ptr LearnIt2TrainerUI::trainer() const {
	return _trainer;
}

/***************************
 * AJAX event handlers
 ***************************/
const void* LearnIt2TrainerUI::event_handler(enum mg_event event,
		struct mg_connection* conn, const struct mg_request_info* request_info)
{
	const void* processed = "yes";

	if (event == MG_NEW_REQUEST) {
		if (!_finished) {
			if (strcmp(request_info->uri, "/ajax/features") == 0) {
				send_ajax_feature_list(conn, request_info);
			} else if (strcmp(request_info->uri, "/ajax/recordFeature") == 0) {
				ajax_record_feature(conn, request_info);
			} else if (strcmp(request_info->uri, "/ajax/penalties") == 0) {
				ajax_get_penalties(conn, request_info);
			} else if (strcmp(request_info->uri, "/ajax/probChart") == 0) {
				ajax_prob_chart(conn, request_info);
			} else if (strcmp(request_info->uri, "/ajax/finish") == 0) {
				ajax_active_learning_done(conn, request_info);
			} else {
				processed = NULL;
			}
		} else {
			processed = NULL;
		}
	} else {
		processed = NULL;
	}

	// if we return NULL, Mongoose will try to serve the request from
	// the specified html directory
	return processed;
}

void LearnIt2TrainerUI::ajax_get_penalties(struct mg_connection* conn,
		const struct mg_request_info* request_info) 
{
//	boost::mutex::scoped_lock l(_active_learning_update_mutex);
	boost::mutex::scoped_lock l(_trainer->updateMutex());

	MongooseUtils::begin_ajax_reply(conn);
	MongooseUtils::send_json(conn, request_info, 
		JSONUtils::jsonify(_trainer->UIDiagnosticString()));
}

void LearnIt2TrainerUI::ajax_prob_chart(struct mg_connection* conn,
		const struct mg_request_info* request_info) 
{
	//boost::mutex::scoped_lock l(_active_learning_update_mutex);
	boost::mutex::scoped_lock l(_trainer->updateMutex());

	MongooseUtils::begin_ajax_reply(conn);
	//ProbChart_ptr prob_chart = trainer()->probChart();

	//if (prob_chart) {	
	MongooseUtils::send_json(conn, request_info, 
		_trainer->probChart().json());
	/*} else {
		MongooseUtils::send_json(conn, request_info, "[]");
	}*/
}

void LearnIt2TrainerUI::ajax_active_learning_done(struct mg_connection* conn,
			const struct mg_request_info* request_info) 
{
	MongooseUtils::begin_ajax_reply(conn);
	stringstream json;
	json << "{}";
	MongooseUtils::send_json(conn, request_info, json.str());
	_finished = true;
	_trainer->stop();
}

void LearnIt2TrainerUI::send_ajax_feature_list(
	struct mg_connection* conn, const struct mg_request_info* request_info)
{
	stringstream json;
	json << "[";
	bool first = true;
	//BOOST_FOREACH(int feat, _features_being_annotated) {
	BOOST_FOREACH(int feat, _featureGrid) {
		if (first) {
			first = false;
		} else {
			json << ", ";
		}
		feature_as_json(feat, json);
	}
	json << "]";
	
	MongooseUtils::begin_ajax_reply(conn);
	MongooseUtils::send_json(conn, request_info, json.str());
}

void LearnIt2TrainerUI::ajax_record_feature(struct mg_connection* conn,
	  const struct mg_request_info* request_info)
{
	// see what feature and label they are sending back
	int id = (size_t)boost::lexical_cast<int>(
		MongooseUtils::get_qsvar(request_info, "id", 100));
	string state = MongooseUtils::get_qsvar(request_info, "state", 100);
	
	if (id < 0 || (size_t)id > _featureGrid.size()) {
		return;
	}

	//_alData->recordFeature(id, state);
	int annFeat = _featureGrid[id];
	_alData->registerAnnotation(annFeat, state);
	_alData->stopAnnotation(annFeat);
	_strategy->label(annFeat);

	// update instance annotation
	updateInstanceAnnotation(request_info->query_string);

	// consider what locking, if any, is necessary here
	stringstream json;
	try {
		int new_feature = _strategy->nextFeature();
		_featureGrid[id] = new_feature;
		_alData->startAnnotation(new_feature);

		feature_as_json(new_feature, json);
		MongooseUtils::begin_ajax_reply(conn);
		MongooseUtils::send_json(conn, request_info, json.str());
	} catch (CouldNotChooseFeatureException) {
		// what should we send do here???
	}
}

void LearnIt2TrainerUI::syncToDB() {
	_trainer->syncToDB();
	_alData->syncToDB();
}

/*****************************
 * Convenience methods
 ******************************/

void LearnIt2TrainerUI::feature_as_json(int feat, std::stringstream& json) {
	string name = JSONUtils::jsonify(
					JSONUtils::html_escape(_trainer->featureName(feat)));
					//JSONUtils::html_escape(_db->getFeatureName(feat)));
	json << "{ \"idx\" : " << feat << ", \"weight\" : " <<
		//parameters()(feat) << ", \"name\": " << name << ", " <<
		_trainer->featureWeight(feat) << ", \"name\": " << name << ", " <<
		"\"instance_data\" : ";
	//JSONUtils::jsonify(json, clientInstances(feat, 10));
	JSONUtils::jsonify(json, clientInstances(feat, 10));
	json << "}";
}

std::vector<ClientInstance> 
LearnIt2TrainerUI::clientInstances(int feat, size_t max_instances) const {
	std::vector<ClientInstance> ret;
	BOOST_FOREACH(int inst, _instanceStrategy->clientInstances(feat, max_instances)) {
		ret.push_back(makeClientInstance(inst));
	}
	return ret;
}

ClientInstance LearnIt2TrainerUI::makeClientInstance(int inst) const {
	//return ClientInstance(_instance_hashes[inst], _preview_strings->getString(inst),
	return ClientInstance(_instanceHashes->hash(inst), previewString(inst),
		_alData->instanceAnnotation(inst));
}

wstring LearnIt2TrainerUI::previewString(int inst) const {
	return _preview_strings->getString(inst);
}

void LearnIt2TrainerUI::updateInstanceAnnotation(const string& encoded) {
	// extracts any updates to instance annotations from the JSON string
	// the client passed us back. 
	// TODO: we should really find some general JSON parsing library to use
	// for this sort of thing. For now, we pull out the data we are about
	// by hand

	// html encoding of something like "instance_data[0][hash]=1234"
	regex hash_regex("instance_data%5B(\\d+)%5D%5Bhash%5D=(\\d+)");
	// html encoding of something like "instance_data[0][annotation]=-1"
	regex annotation_regex("instance_data%5B(\\d+)%5D%5Bannotation%5D=([0-9-]+)");

	typedef std::map<int, size_t> InstanceIDToHashMap;
	typedef std::map<int, int> InstanceIDToAnnotationMap;
	InstanceIDToHashMap instIDToHash;
	InstanceIDToAnnotationMap instIDToAnnotation;

	string::const_iterator start, end;
	match_results<string::const_iterator> what;

	start = encoded.begin();
	end = encoded.end();

	// gather which client ids are associated with which instance ids (=hashes)
	while (regex_search(start, end, what, hash_regex)) {
		instIDToHash.insert(make_pair(
			lexical_cast<int>(string(what[1].first, what[1].second)),
			lexical_cast<size_t>(string(what[2].first, what[2].second))));
		// continue search after the end of this match (what[0] is full match)
		start = what[0].second;
	}

	// gather which client ids are associated with which annotations
	start = encoded.begin();
	end = encoded.end();
	while (regex_search(start, end, what, annotation_regex)) {
		instIDToAnnotation.insert(make_pair(
			lexical_cast<int>(string(what[1].first, what[1].second)),
			lexical_cast<int>(string(what[2].first, what[2].second))));
		// continue search after the end of this match (what[0] is full match)
		start = what[0].second;
	}

	if (instIDToHash.size() != instIDToAnnotation.size()) {
		SessionLogger::warn("LearnIt2") << "In instance annotation received "
			<< "from client, number of parsed hashes does not agree with "
			<< "number of parsed annotations.";
	}

	ActiveLearningData::InstanceHashToAnnotationMap hashToAnnMap;

	BOOST_FOREACH_PAIR(int id, size_t hash, instIDToHash) {
		InstanceIDToAnnotationMap::const_iterator probe = 
			instIDToAnnotation.find(id);
		if (probe != instIDToAnnotation.end()) {
			int annotation = probe->second;
			// we don't immediately update _annotatedInstances due to 
			// threading issues. Instead, we just queue these up to be
			// processed between iterations.
			// check if we already have an annotation for this instance
			//_newAnnotatedInstances.push_back(AnnotatedInstanceRecord(hash, annotation));
			hashToAnnMap.insert(make_pair(hash, annotation));
		} else {
			SessionLogger::warn("LearnIt2") << "In instance annotation received "
				<< "from client, could not find annotation for hash entry.";
		}
	}

	_alData->updateInstanceAnnotations(hashToAnnMap);
}

bool LearnIt2TrainerUI::finished() const {
	return _finished;
}

