#ifndef _TEMPORAL_TRAINER_UI_
#define _TEMPORAL_TRAINER_UI_

#include <string>
#include <boost/shared_ptr.hpp>
#include "mongoose/mongoose.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "ActiveLearning/FeatureGrid.h"

BSP_DECLARE(TemporalTrainerUI)
BSP_DECLARE(TemporalTrainer)
BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(ActiveLearningStrategy)
BSP_DECLARE(InstanceStrategy)
BSP_DECLARE(StringStore)
BSP_DECLARE(InstanceHashes)
class ClientInstance;

// TODO: refactor this and LearnIt2TrainerUI to move common code to base class
class TemporalTrainerUI {
public:
	const void* event_handler(enum mg_event event,
		struct mg_connection* conn, const struct mg_request_info *request_info);
	bool updateBetweenIterations();
	bool finished() const;

	TemporalTrainer_ptr trainer();
	void syncToDB();
private:
	TemporalTrainerUI(TemporalTrainer_ptr trainer, ActiveLearningData_ptr alData,
		ActiveLearningStrategy_ptr strategy, InstanceStrategy_ptr instanceStrategy,
		InstanceHashes_ptr instanceHashes, StringStore_ptr preview_strings);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(TemporalTrainerUI, TemporalTrainer_ptr,
		ActiveLearningData_ptr, ActiveLearningStrategy_ptr, InstanceStrategy_ptr,
		InstanceHashes_ptr, StringStore_ptr);

	/**********************
	 * AJAX event handlers
	 **********************/
	void ajax_get_penalties(struct mg_connection* conn,
		const struct mg_request_info* request_info);
	/*void ajax_prob_chart(struct mg_connection* conn,
		const struct mg_request_info* request_info);*/
	void ajax_active_learning_done(struct mg_connection* conn,
			const struct mg_request_info* request_info);
	void send_ajax_feature_list(struct mg_connection* conn, 
		const struct mg_request_info* request_info);
	void ajax_record_feature(struct mg_connection* conn,
	  const struct mg_request_info* request_info);

	/*********************
	 * Convenience methods
	 *********************/
	void updateInstanceAnnotation(const std::string& encoded);
	void feature_as_json(int feat, std::stringstream& json);

	// initialization
	void populateFeatureGrid();

	/*********************************************
	 * Information to support instance annotation
	 *********************************************/
	StringStore_ptr _preview_strings;
	std::wstring previewString(int inst) const;
	ClientInstance makeClientInstance(int inst) const;
	// pick random instances of the specified feature and return the
	// information needed to annotate them
	std::vector<ClientInstance> clientInstances(int feat, size_t max_instances) const;

private:
	FeatureGrid _featureGrid;
	ActiveLearningData_ptr _alData;
	TemporalTrainer_ptr _trainer;
	bool _finished;
	InstanceHashes_ptr _instanceHashes;

	// the strategy used to determine in what order to annotate features
	ActiveLearningStrategy_ptr _strategy;
	InstanceStrategy_ptr _instanceStrategy;
};

#endif
