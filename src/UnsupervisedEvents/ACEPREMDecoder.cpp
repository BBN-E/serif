#include "Generic/common/leak_detection.h"

#include "ACEPREMDecoder.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "LearnIt/util/FileUtilities.h"
#include "GraphicalModels/DataSet.h"
#include "GraphicalModels/Alphabet.h"
#include "GraphicalModels/pr/Constraint.h"
#include "GraphicalModels/pr/FeatureImpliesClassConstraint.h"
#include "GraphicalModels/pr/ClassConstantUpperBoundsConstraint.h"
#include "GraphicalModels/pr/ClassConstantLowerBoundsConstraint.h"
#include "GraphicalModels/distributions/MultinomialDistribution.h"
#include "GraphicalModels/io/Reporter.h"
#include "GraphicalModels/learning/PREM.h"
#include "GraphicalModels/pr/FeatureImpliesClassConstraint.h"
#include "GraphicalModels/pr/ClassRequiresFeature.h"
#include "ProblemDefinition.h"
#include "ACEEventDumper.h"
#include "ACEPassageDescription.h"
#include "AnswerMatrix.h"
#include "ACEEvent.h"

using std::string;
using std::wstring;
using std::wcout;
using std::endl;
using boost::make_shared;
using boost::lexical_cast;
using boost::split;
using boost::is_any_of;
using GraphicalModel::Alphabet;
using GraphicalModel::DataSet;
using GraphicalModel::MultinomialDistribution;
using GraphicalModel::AsymmetricDirichlet;
using GraphicalModel::SymmetricDirichlet;
using GraphicalModel::Constraint;
using GraphicalModel::ConstraintsType;
using GraphicalModel::InstanceConstraintsType;
using GraphicalModel::FeatureImpliesClassConstraint;
using GraphicalModel::ClassConstantUpperBoundConstraint;
using GraphicalModel::ClassConstantLowerBoundConstraint;
using GraphicalModel::FeatureImpliesClassConstraint;
using GraphicalModel::ClassRequiresFeature;

ACEPREMDecoder::ACEPREMDecoder(const ProblemDefinition_ptr& problem,
		const std::string& trainDocTable, const std::string& testDocTable)
: _problem(problem)
{
	ACEEvent::loadFromDocTable(trainDocTable, *_problem, _aceEvents,
			ParamReader::getRequiredIntParam("max_training_docs"));
	ACEEvent::loadFromDocTable(testDocTable, *_problem, _aceEvents);

	SessionLogger::info("total_events") << L"Loaded a total of " 
		<< _aceEvents.nGraphs() << L" ACE events";

	if (ParamReader::getRequiredTrueFalseParam("use_soft_entity_constraints")) {
		SessionLogger::info("soft_entity_constraints") 
			<< "Using soft entity constraints.";
	} else {
		SessionLogger::info("soft_entity_constraints") 
			<< "Not using soft entity constraints.";
	}

	if (ParamReader::getRequiredTrueFalseParam("use_instance_constraints")) {
		SessionLogger::info("instance_constraints")
			<< "Using instance constraints.";
	} else {
		SessionLogger::info("instance_constraints")
			<< "Not using instance constraints.";
	}

	ACEEvent::finishedLoading(*_problem);

	createCorpusConstraints();
	createInstanceConstraints();

	_problem->setInstanceConstraints(_instanceConstraints);
	_problem->setCorpusConstraints(_constraints);

	createKeyToEventMap(); 
}

void ACEPREMDecoder::train() {
	SessionLogger::info("begin_prem") << "Beginning PREM training";
	GraphicalModel::Reporter<ACEEvent, ProblemDefinition, ACEEventDumper, ACEModelDumper> 
		reporter(ParamReader::getRequiredParam("output_directory"), _problem);
	GraphicalModel::PREM<ACEEvent> prModel(_aceEvents, _problem->nClasses(), 
			_constraints, _instanceConstraints,
			GraphicalModel::PREM<ACEEvent>::LBFGS, true);
	prModel.em(_aceEvents, 10, reporter);
	bool ignore_instance_constraints = 
		!ParamReader::getRequiredTrueFalseParam("use_instance_constraints_in_final_decoding");
	ACEInstanceConstraints_ptr oldInstConstraints = _instanceConstraints;
	if (ignore_instance_constraints) {
		SessionLogger::info("delete_inst") << "Deleting instance constraints before final decoding";
		_instanceConstraints = boost::make_shared<ACEInstanceConstraints>(
					_aceEvents.nGraphs(), _constraints->size());
		_instanceConstraints->freeze();
		prModel.resetInstanceConstraints(_instanceConstraints);
	}
	prModel.e(_aceEvents);
	if (ignore_instance_constraints) {
		_instanceConstraints = oldInstConstraints;
		prModel.resetInstanceConstraints(_instanceConstraints);
	}
	const string final = "final";
	reporter.postE(final, _aceEvents);
	SessionLogger::info("end_prem") << "Ending PREM training";
}

void parseLegalEntities(const std::wstring& constraintData, 
		std::vector<unsigned int>& roles, std::vector<std::wstring>& entities,
		ProblemDefinition& problem)
{
	static const boost::wregex part_re(L"(\\w+)\\[([A-Z_,]+)\\]");
	boost::match_results<std::wstring::const_iterator> what;
	if (boost::regex_match(constraintData, what, part_re)) {
		wstring className(what[1].first, what[1].second);
		roles.push_back(problem.classNumber(className));
		wstring entityTypesStr(what[2].first, what[2].second);
		split(entities, entityTypesStr, is_any_of(L","));
	} else {
		std::wstringstream err;
		err << L"Bad legal entity definition " << constraintData;
		throw UnexpectedInputException("ACEPREMDecoder - parseLegalEntities",
				err);
	}

}

void ACEPREMDecoder::createCorpusConstraints() {
	_constraints = make_shared<ACEConstraints>();

	static const wstring HWIC = L"HeadWordImpliesClass";
	static const wstring PDIC = L"PropDepImpliesClass";
	static const wstring ETIC = L"EntityTypeImpliesClass";
	static const wstring LEGAL_ENTITIES = L"LegalEntities";

	typedef FeatureImpliesClassConstraint<HeadWordConstraintView> FICC_HW;
	typedef FeatureImpliesClassConstraint<PropDepConstraintView> FICC_PD;
	typedef FeatureImpliesClassConstraint<EntityTypeConstraintView> FICC_ET;

	std::vector<wstring> classParts;
	std::vector<wstring> featureParts;
	std::vector<wstring> lineParts;
	std::vector<unsigned int> classIDs;

	BOOST_FOREACH(const ConstraintDescription& desc, _problem->constraintDescriptions()) {
		if (HWIC == desc.constraintType || PDIC == desc.constraintType
				|| ETIC == desc.constraintType)
		{
			GraphicalModel::FICCDescription parsedDesc = 
				GraphicalModel::parseFICCDescription(desc.constraintData);
			std::vector<unsigned int> lookedupClasses;

			BOOST_FOREACH(const std::wstring& classStr, parsedDesc.classes) {
				lookedupClasses.push_back(_problem->classNumber(classStr));
			}

			if (HWIC == desc.constraintType) {
				_constraints->push_back(make_shared<FICC_HW>(_aceEvents, 
							parsedDesc.features, lookedupClasses, 
							desc.weight, HeadWordConstraintView()));
			} else if (PDIC == desc.constraintType) {
				_constraints->push_back(make_shared<FICC_PD>(_aceEvents, 
							parsedDesc.features,  lookedupClasses,
							desc.weight, PropDepConstraintView()));
			} else if (ETIC == desc.constraintType) {
				if (ParamReader::getRequiredTrueFalseParam("use_soft_entity_constraints")) {
					_constraints->push_back(make_shared<FICC_ET>(_aceEvents, 
								parsedDesc.features, lookedupClasses, 
								desc.weight, EntityTypeConstraintView()));
				}
			} else {
				throw UnexpectedInputException("createConstraintsACE",
						"Impossible constraint type; programming error");
			}
		} else if (LEGAL_ENTITIES == desc.constraintType) {
			std::vector<unsigned int> roles;
			std::vector<std::wstring> entities;
			parseLegalEntities(desc.constraintData, roles, entities, *_problem);
			_constraints->push_back(
					make_shared<ClassRequiresFeature<EntityTypeConstraintView> >(
						_aceEvents, entities, roles, .01, EntityTypeConstraintView())); 
		} else {
			wstringstream err;
			err << L"Unrecognized constraint type: '" << desc.constraintType
				<< L"'";
			throw UnexpectedInputException("createConstraints", err);
		}
	}

	SessionLogger::info("loaded_constraints") << 
		L"Loaded " << _constraints->size() << L" constraints";
}


template <class ConstraintType>
void addInstanceConstraint(const InstanceConstraintDescription& desc,
		const ProblemDefinition& problem, 
		const GraphicalModel::DataSet<ACEEvent>& aceEvents,
		GraphicalModel::InstanceConstraintsCollection<ACEEvent>& instanceConstraints)
{
	std::vector<std::wstring> parts;
	split(parts, desc.constraintData, is_any_of(L";"));

	double limit = lexical_cast<double>(parts[0]);
	typename ConstraintType::Classes classes;
	for (size_t i=1; i<parts.size(); ++i) {
		classes.push_back(problem.classNumber(parts[i]));
	}

	for (size_t i=0; i<aceEvents.nGraphs(); ++i) {
		// doesn't matter what view we use
		instanceConstraints.addConstraintForInstance(i,
				make_shared<ConstraintType>(*aceEvents.graphs[i], classes, 
					limit, ACEDummyFactorView()));
	}

	instanceConstraints.addConstraintName(desc.name);
}

void ACEPREMDecoder::createInstanceConstraints()
{
	_instanceConstraints = make_shared<ACEInstanceConstraints>(
				_aceEvents.nGraphs(), _constraints->size());

	if (ParamReader::getRequiredTrueFalseParam("use_instance_constraints")) {
		BOOST_FOREACH(const InstanceConstraintDescription& desc, 
				_problem->instanceConstraintDescriptions())
		{
			SessionLogger::info("instance_constraints") << L"Loading instance constraints.";

			if (desc.constraintType == L"upper") {
				addInstanceConstraint<ClassConstantUpperBoundConstraint<ACEDummyFactorView> >
					(desc, *_problem, _aceEvents, *_instanceConstraints);
			} else if (desc.constraintType == L"lower") {
				addInstanceConstraint<ClassConstantLowerBoundConstraint<ACEDummyFactorView> >
					(desc, *_problem, _aceEvents, *_instanceConstraints);
			} else {
				wstringstream err;
				err << L"Unknown instance constraint type " << desc.constraintType;
				throw UnexpectedInputException("ACEPREMDecoder::createInstanceConstraints",
						err);
			}
		}
	}

	_instanceConstraints->freeze();
}

void ACEPREMDecoder::createKeyToEventMap() {
	for (size_t i=0; i<_aceEvents.nGraphs(); ++i) {
		_keyToEventMap.insert(make_pair(_aceEvents.graphs[i]->passage().key(), i));
	}
}

ACEEvent_ptr ACEPREMDecoder::eventByKey(const std::wstring& key) const {
	KeyToEventMap::const_iterator probe = _keyToEventMap.find(key);

	if (probe != _keyToEventMap.end()) {
		return _aceEvents.graphs[probe->second];
	} else {
		std::wstringstream err;
		err << L"The document set the PREM trainer ran on did not contain "
			<< L"an event matching the key " << key;
		throw UnexpectedInputException("ACEPREMDecoder::eventByKey", err);
	}

}

PREMAdapter::IdxScore PREMAdapter::maxOverSentence(const ACEEvent& event, size_t label) 
{
	unsigned int max_idx = -1;
	double max_val = 0.0;
	for (unsigned int i = 0; i<event.nEntities(); ++i) {
		if (event.probsOfEntity(i)[label] > max_val) {
			max_val = event.probsOfEntity(i)[label];
			max_idx = i;
		}
	}

	return make_pair((int)max_idx, max_val);
}

PREMAdapter::IdxScore PREMAdapter::maxAvailableOverSentence(const ACEEvent& event, 
		size_t role, const AnswerMatrix& ret) 
{
	unsigned int max_idx = -1;
	double max_val = 0.0;
	for (unsigned int i = 0; i<event.nEntities(); ++i) {
		if (event.probsOfEntity(i)[role] > max_val
				&& !ret.assignedARole(i))
		{
			max_val = event.probsOfEntity(i)[role];
			max_idx = i;
		}
	}

	return make_pair((int)max_idx, max_val);
}

ACEEvent_ptr PREMAdapter::matchingEvent(const ACEEvent& event) const {
	return _premDecoder->eventByKey(event.passage().key());
}


