#ifndef _LEARNIT2_TRAINER_H_
#define _LEARNIT2_TRAINER_H_

#include <string>
#include <vector>
//#include <list>
#include <boost/thread/mutex.hpp>
//#include "mongoose/mongoose.h"

#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/Eigen/Core"
// the following define is not a problem for us - the parts which aren't
// finalized are not parts we use
/*#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include "learnit/Eigen/Sparse"*/
//#include "learnit/lbfgs/FunctionWithGradient.h"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"
#include "ProbChart.h"

BSP_DECLARE(LearnIt2DB)
BSP_DECLARE(FeatureAlphabet)
BSP_DECLARE(LearnIt2Trainer)
BSP_DECLARE(StringStore)
BSP_DECLARE(ActiveLearningStrategy)
BSP_DECLARE(ProbChart)
BSP_DECLARE(AnnotatedFeatureRecord)
BSP_DECLARE(ActiveLearningData)
BSP_DECLARE(LearnIt2TrainerUI)
BSP_DECLARE(InferenceDataView)
BSP_DECLARE(OptimizationView)

class AnnotatedInstanceRecord;
class ClientInstance;

BSP_DECLARE(PostIterationCallback)
class PostIterationCallback {
public:
	virtual bool operator()()=0;
};

class LearnIt2Trainer {
public:
	/********************
	 * Core methods
	 *******************/
	// the objective function starts empty and you add terms to it
	// using this method
	void addObjectiveComponent(InstancewiseObjectiveFunctionComponent_ptr objComp);
	// not for user use - to be called by optimization routines between
	// iterations
	virtual bool postIterationHook();
	// register a PostIterationCallback function object here to have it
	// called between each iteration
	void registerPostIterationHook(PostIterationCallback_ptr cb);
	// do the actual learning.  If active learning is off, it will just
	// run for the number of iterations specified in the parameter file.
	// If active learning is on, it will run until stop() is call plus
	// the number of iterations in the parameter file
	void optimize(int max_iterations);
	void optimize();
	void stop();	
	// call when training is done to write weights back to the database.
	void syncToDB();
		
	/************
	 * Accessors
	 ************/
	int nFeatures() const;
	std::wstring featureName(int feature) const;
	double featureWeight(int feature) const;

	LearnIt2DB& database() const;
	// this mutex is locked while learning is underway, so you can
	// use it to guarantee you're seeing the data in a state in between
	// iterations and not in the middle of a line search
	boost::mutex& updateMutex();

	/*****************
	 * Other
	 ****************/
	// a diagnostic string showing the state of the objective function
	std::wstring UIDiagnosticString() const;
	// a chart showing the probability distributions of the two models
	const ProbChart& probChart() const;
	void dump(std::string& output_file) const;

	/***************
	 * testing only
	 **************/
	void _test_clear_objective_components();
private:
	LearnIt2Trainer(LearnIt2DB_ptr db, FeatureAlphabet_ptr alphabet,
		OptimizationView_ptr slotView, OptimizationView_ptr sentenceView, 
		unsigned int n_instances, bool active_learning);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(LearnIt2Trainer, LearnIt2DB_ptr,
		FeatureAlphabet_ptr, OptimizationView_ptr, OptimizationView_ptr, 
		unsigned int, bool);

	void calcUIDiagnosticString();

	OptimizationView_ptr _sentenceView;
	OptimizationView_ptr _slotView;
	
	std::vector<PostIterationCallback_ptr> _postIterationHooks;
	
	LearnIt2DB_ptr _db;
	FeatureAlphabet_ptr _alphabet;
	bool _canFinish;
	
	unsigned int _iteration;
	unsigned int _nParams;
	boost::mutex _active_learning_update_mutex;
	ProbChart _prob_chart;

	double _global_objective;

	void setDirty();
	/*int _debug_e;
	bool _sgd;
	double _sgd_q;
	double _regLoss;
	double _debug_reg_loss;
	Eigen::VectorXi _lastRegularized;

	bool _verbose;*/
};

#endif
