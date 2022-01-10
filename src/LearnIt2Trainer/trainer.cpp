#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <wchar.h>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/foreach_pair.hpp"
#include "ActiveLearning/AnnotatedInstanceRecord.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearning/DataView.h"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/strategies/ActiveLearningStrategy.h"
#include "ActiveLearning/strategies/RandomStrategy.h"
#include "ActiveLearning/strategies/CoverageStrategy.h"
#include "ActiveLearning/ui/ClientInstance.h"
#include "ActiveLearning/ui/JSONUtils.h"
#include "ActiveLearning/ui/MongooseUtils.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/lbfgs/lbfgs.h"
#include "LearnIt/Eigen/Core"
#include "LearnIt/Eigen/Sparse"
#include "LearnIt/db/LearnIt2DB.h"
#include "OptimizationView.h"
#include "trainer.h"
#include "ProbChart.h"

using namespace Eigen;
using boost::make_shared;
using std::wstring; using std::string;

LearnIt2Trainer::LearnIt2Trainer(LearnIt2DB_ptr db, FeatureAlphabet_ptr alphabet,
	OptimizationView_ptr slotView, OptimizationView_ptr sentenceView, 
	unsigned int n_instances, bool active_learning) :
		_db(db), _nParams(slotView->nParams()),
		_alphabet(alphabet), _sentenceView(sentenceView), _slotView(slotView),
		_canFinish(!active_learning),
		_iteration(0), _prob_chart(_slotView->data(), _sentenceView->data())
{
	if (db->readOnly()) {
		throw UnexpectedInputException("LearnIt2Trainer::LearnIt2Trainer",
			"Cannot construct LearnIt2Trainer from a read-only LearnIt DB");
	}
};



void LearnIt2Trainer::registerPostIterationHook(PostIterationCallback_ptr cb) {
	_postIterationHooks.push_back(cb);
}

void LearnIt2Trainer::syncToDB() {
	VectorXd tmpParams(_nParams);

	for (unsigned int i=0; i<_nParams; ++i) {
		if (_sentenceView->ownsFeature(i)) {
			tmpParams(i) = _sentenceView->parameters()(i);
		} else {
			tmpParams(i) = _slotView->parameters()(i);
		}
	}

	_alphabet->setFeatureWeights(tmpParams);
}

void LearnIt2Trainer::optimize() {
	optimize(ParamReader::getRequiredIntParam("max_trainer_iterations"));
}

void LearnIt2Trainer::optimize(int max_iterations) {
	int max_view_iterations = ParamReader::getOptionalIntParamWithDefaultValue(
			"max_view_iterations", 1000);
	double global_objective_change_threshold = ParamReader::getOptionalFloatParamWithDefaultValue(
			"global_objective_change_threshold", 0.01);
	double lbfgs_gradient_tolerance = ParamReader::getOptionalFloatParamWithDefaultValue(
			"lbfgs_gradient_tolerance", 1e-3);
	double lbfgs_objective_tolerance = ParamReader::getOptionalFloatParamWithDefaultValue(
			"lbfgs_objective_tolerance", 0.0);
	int outer_iterations_after_can_finish = ParamReader::getOptionalIntParamWithDefaultValue(
			"iterations_after_annotation", 25);


	SessionLogger::info("remember_to_cite") << L"Remember to cite " 
		<< L"Massih-Reza Amini & Cyril Goutte. \"A co-classification "
		<< L"approach to learning from multilingual corpora.\". Machine Learning"
		<< L" (2010) 79:105-121";

	int iterations_after_annotation = 0;
	while (!_canFinish) {
		double old_global_objective = -std::numeric_limits<double>::max();
		
		_sentenceView->inference();
		_slotView->inference();
		_sentenceView->optimize(max_view_iterations, lbfgs_gradient_tolerance,
				lbfgs_objective_tolerance);
		_slotView->optimize(max_view_iterations, lbfgs_gradient_tolerance,
				lbfgs_objective_tolerance);
		_global_objective = _sentenceView->value() + _slotView->value();

		SessionLogger::info("global_progress") << 
			_iteration << L": g: " << _global_objective << L"; sent: " <<
			_sentenceView->value() << "; slot: " << _slotView->value();

		while (fabs(_global_objective - old_global_objective) > global_objective_change_threshold) {
			if (iterations_after_annotation > outer_iterations_after_can_finish) {
				break;
			}
			++_iteration;
			if (_canFinish) {
				++iterations_after_annotation;
			}
			postIterationHook();
			_sentenceView->optimize(max_view_iterations, lbfgs_gradient_tolerance,
					lbfgs_objective_tolerance);
			_sentenceView->inference();
			_slotView->optimize(max_view_iterations, lbfgs_gradient_tolerance,
					lbfgs_objective_tolerance);
			_slotView->inference();
			old_global_objective = _global_objective;
			_global_objective = _sentenceView->value() + _slotView->value();
			SessionLogger::info("global_progress") << 
			_iteration << L"(" << iterations_after_annotation << L"): g: " << _global_objective << L"; sent: " <<
			_sentenceView->value() << "; slot: " << _slotView->value();
		}
	}
}

bool LearnIt2Trainer::postIterationHook() {
	bool ret = false;
	
	BOOST_FOREACH(PostIterationCallback_ptr callback, _postIterationHooks) {
		if ((*callback)()) {
			setDirty();
			ret = true;
		}
	}

	_sentenceView->betweenIterations();
	_slotView->betweenIterations();

	_prob_chart.observe();

	++_iteration;
	return ret;
}

void LearnIt2Trainer::setDirty() {
	_sentenceView->setDirty();
	_slotView->setDirty();
}

int LearnIt2Trainer::nFeatures() const {
	return _nParams;
}

LearnIt2DB& LearnIt2Trainer::database() const {
	return *_db;
}

wstring LearnIt2Trainer::UIDiagnosticString() const {
	wstringstream str;
	str << L"Sent View: " << _sentenceView->UIDiagnosticString() << L"<br/>";
	str << L"Slot View: " << _slotView->UIDiagnosticString() << L"<br/>";
	str << L"Global: " << _global_objective;

	return str.str();
}

const ProbChart& LearnIt2Trainer::probChart() const {
	return _prob_chart;
}

boost::mutex& LearnIt2Trainer::updateMutex() {
	return _active_learning_update_mutex;
}

void LearnIt2Trainer::stop() {
	_canFinish = true;
}

double LearnIt2Trainer::featureWeight(int feat) const {
	if (_sentenceView->ownsFeature(feat)) {
		return _sentenceView->parameters()(feat);
	} else {
		return _slotView->parameters()(feat);
	}
}

std::wstring LearnIt2Trainer::featureName(int feat) const {
	return _alphabet->getFeatureName(feat);
}

void LearnIt2Trainer::_test_clear_objective_components() {
	_sentenceView->_test_clear_objective_components();
	_slotView->_test_clear_objective_components();
}
