#ifndef _TEMPORAL_TRAINER_H_
#define _TEMPORAL_TRAINER_H_

#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "learnit/Eigen/Core"
#include "learnit/lbfgs/FunctionWithGradient.h"

BSP_DECLARE(TemporalTrainer)
BSP_DECLARE(ObjectiveFunctionComponent)
BSP_DECLARE(PostIterationCallback)
BSP_DECLARE(MultiAlphabet)
BSP_DECLARE(TemporalDB)
BSP_DECLARE(MultilabelInferenceDataView)
BSP_DECLARE(TemporalTypeTable)

BSP_DECLARE(PostIterationCallback)
class PostIterationCallback {
public:
	virtual bool operator()()=0;
};

class TemporalTrainer : public FunctionWithGradient {
public:
	void addObjectiveComponent(ObjectiveFunctionComponent_ptr objComp);
	void registerPostIterationHook(PostIterationCallback_ptr cb);
	void syncToDB();

	bool postIterationHook();
	void optimize(unsigned int max_iterations);
	void optimize();

	unsigned int nFeatures() const;
	TemporalDB& database() const;
	boost::mutex& updateMutex();
	void stop();
	double featureWeight(unsigned int feat) const;
	std::wstring featureName(unsigned int feat) const;
	std::wstring featureAttribute(unsigned int feat) const;
	std::wstring UIDiagnosticString() const;
	MultiAlphabet_ptr alphabet() const;
	double prediction(int inst) const;

private:
	TemporalTrainer(TemporalDB_ptr db, MultiAlphabet_ptr alphabet,
		MultilabelInferenceDataView_ptr data, bool activeLearning);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(TemporalTrainer, TemporalDB_ptr,
		MultiAlphabet_ptr, MultilabelInferenceDataView_ptr, bool);

	void recalc(const Eigen::VectorXd& params, double& value,
		Eigen::VectorXd& gradient) const;

	void inference(const Eigen::VectorXd& params) const;

	MultilabelInferenceDataView_ptr _data;
	std::vector<PostIterationCallback_ptr> _postIterationHooks;
	std::vector<ObjectiveFunctionComponent_ptr> _objectiveComponents;
	TemporalDB_ptr _db;
	MultiAlphabet_ptr _alphabet;
	bool _canFinish;
	mutable double _ll;
	double _debug_ll;
	int _iteration;
	boost::mutex _active_learning_update_mutex;
	TemporalTypeTable_ptr _typeTable;
};

#endif
