#include "Generic/common/leak_detection.h"
#include "TemporalTrainer.h"

#include <boost/foreach.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ActiveLearning/MultilabelInferenceDataView.h"
#include "ActiveLearning/objectives/ObjectiveFunctionComponent.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "learnit/lbfgs/lbfgs.h"
#include "temporal/TemporalDB.h"
#include "temporal/TemporalTypeTable.h"

using Eigen::VectorXd;

// TODO: at some point this and LearnIt2Trainer/trainer.cpp should be
// refactored and the common code put in a base class

TemporalTrainer::TemporalTrainer(TemporalDB_ptr db, MultiAlphabet_ptr alphabet,
		MultilabelInferenceDataView_ptr data, bool activeLearning) 
: FunctionWithGradient(alphabet->size()), _canFinish(!activeLearning),
_alphabet(alphabet), _data(data), _db(db), _iteration(0), _ll(0.0),
	_typeTable(db->makeTypeTable())
{
	setParams(alphabet->getWeights());
	/*if (db->readOnly()) {
		throw UnexpectedInputException("TemporalTrainer::TemporalTrainer",
			"Cannot construct TemporalTrainer from a read-only temporal DB");
	}*/
}

MultiAlphabet_ptr TemporalTrainer::alphabet() const {
	return _alphabet;
}

void TemporalTrainer::addObjectiveComponent(ObjectiveFunctionComponent_ptr objComp) {
	_objectiveComponents.push_back(objComp);
}

void TemporalTrainer::registerPostIterationHook(PostIterationCallback_ptr cb) {
	_postIterationHooks.push_back(cb);
}

void TemporalTrainer::inference(const VectorXd& params) const {
	_data->inference(params);
}

void TemporalTrainer::recalc(const Eigen::VectorXd& params, double& value, 
							 Eigen::VectorXd& gradient) const
{
	gradient.setZero(nParams());
	inference(params);

	_ll = 0.0;
	BOOST_FOREACH(ObjectiveFunctionComponent_ptr objComp, _objectiveComponents) {
		_ll += (*objComp)(gradient);
	}

	if (_ll != _ll) {
		SessionLogger::err("blowup") << "Log-liklihood calculate blew up";
	}

	value = _ll;
}

void TemporalTrainer::syncToDB() {
	_alphabet->setFeatureWeights(parameters());
}

void TemporalTrainer::optimize() {
	optimize(ParamReader::getRequiredIntParam("max_temporal_trainer_iterations"));
}

void TemporalTrainer::optimize(unsigned int maxIterations) {
	while (!_canFinish) {
		LBFGS::optimize(*this, 10);
	}
	LBFGS::optimize(*this, maxIterations);
}

bool TemporalTrainer::postIterationHook() {
	bool ret = false;
	
	BOOST_FOREACH(PostIterationCallback_ptr callback, _postIterationHooks) {
		if ((*callback)()) {
			setDirty();
			ret = true;
		}
	}

	_ll = value();
	_debug_ll = _ll;
	//_debug_reg_loss = _regLoss;
	BOOST_FOREACH(const ObjectiveFunctionComponent_ptr obj, _objectiveComponents) {
		obj->snapshot();
	}
//	_prob_chart.observe();

	if (_iteration % 50 == 0) {
		SessionLogger::info("learning_progress") << UIDiagnosticString();
		SessionLogger::info("learning_progress") << "Param norm: " << parameters().norm();
	}
	++_iteration;
	return ret;
}

unsigned int TemporalTrainer::nFeatures() const {
	return parameters().size();
}

TemporalDB& TemporalTrainer::database() const {
	return *_db;
}

wstring TemporalTrainer::UIDiagnosticString() const {
	wstringstream str;
	str << L"iteration: " << _iteration << L": loss= " << _debug_ll << L"(";
	BOOST_FOREACH(ObjectiveFunctionComponent_ptr objComp, _objectiveComponents) {
		str << objComp->status() << L"; ";
	}
	str << L")";

	return str.str();
}

boost::mutex& TemporalTrainer::updateMutex() {
	return _active_learning_update_mutex;
}

void TemporalTrainer::stop() {
	_canFinish = true;
}

double TemporalTrainer::featureWeight(unsigned int feat) const {
	return parameters()(feat);
}

std::wstring TemporalTrainer::featureName(unsigned int feat) const {
	return _alphabet->getFeatureName(feat);
}

std::wstring TemporalTrainer::featureAttribute(unsigned int feat) const {
	return _typeTable->name(_alphabet->dbNumForIdx(feat));
}

double TemporalTrainer::prediction(int inst) const {
	return _data->prediction(inst);
}
