#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "ActiveLearning/FeatureGrid.h"
#include "ActiveLearning/InstanceHashes.h"
#include "mongoose/mongoose.h" // for mg_event

BSP_DECLARE(LearnIt2Trainer)
BSP_DECLARE(LearnIt2TrainerUI)
BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(ActiveLearningStrategy)
BSP_DECLARE(InstanceStrategy)
BSP_DECLARE(StringStore)
class ClientInstance;

class LearnIt2TrainerUI {
public:
	const void* event_handler(enum mg_event event,
		struct mg_connection* conn, const struct mg_request_info* request_info);
	bool updateBetweenIterations();
	bool finished() const;
	LearnIt2Trainer_ptr trainer() const;
	void syncToDB();
private:
	LearnIt2TrainerUI(LearnIt2Trainer_ptr trainer, ActiveLearningData_ptr alData,
		ActiveLearningStrategy_ptr strategy, InstanceStrategy_ptr instanceStrategy,
		InstanceHashes_ptr instanceHashes, StringStore_ptr previewStrings);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(LearnIt2TrainerUI, LearnIt2Trainer_ptr,
		ActiveLearningData_ptr, ActiveLearningStrategy_ptr, InstanceStrategy_ptr,
		InstanceHashes_ptr, StringStore_ptr);

	/**********************
	 * AJAX event handlers
	 **********************/
	void ajax_get_penalties(struct mg_connection* conn,
		const struct mg_request_info* request_info);
	void ajax_prob_chart(struct mg_connection* conn,
		const struct mg_request_info* request_info);
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
	LearnIt2Trainer_ptr _trainer;
	bool _finished;
	InstanceHashes_ptr _instanceHashes;

	// the strategy used to determine in what order to annotate features
	ActiveLearningStrategy_ptr _strategy;
	InstanceStrategy_ptr _instanceStrategy;
};
