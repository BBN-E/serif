// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/relations/es_RelationModel.h"
#include "Spanish/relations/es_OldRelationFinder.h"
#include "Generic/relations/VectorModel.h"
#include "Spanish/relations/es_TreeModel.h"
#include "Spanish/relations/es_PatternMatcherModel.h"
#include "Generic/common/Symbol.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/relations/RelationTypeSet.h"
#include "math.h"
#include "Generic/common/ParamReader.h"

Symbol SpanishRelationModel::forcedOrgSym = Symbol(L"FORCED_ORG");
Symbol SpanishRelationModel::forcedPerSym = Symbol(L"FORCED_PER");

SpanishRelationModel::SpanishRelationModel()
{
	USE_PATTERNS = false;
	USE_METONYMY = false;

	if (!USE_PATTERNS) {
		_vectorModel = _new VectorModel();
		_treeModel = _new TreeModel();
	}
}

void SpanishRelationModel::train(char *training_file, char* output_file_prefix)
{
	if (!USE_PATTERNS) {
		SessionLogger::info("SERIF") << "training on " << training_file << std::endl;
		_vectorModel->train(training_file, output_file_prefix);
		_treeModel->train(training_file, output_file_prefix);
	}
}

void SpanishRelationModel::trainFromStateFileList(char *training_file, char* output_file_prefix)
{
	if (!USE_PATTERNS) {
		SessionLogger::info("SERIF") << "training on " << training_file << std::endl;
		_vectorModel->trainFromStateFileList(training_file, output_file_prefix);
		_treeModel->trainFromStateFileList(training_file, output_file_prefix);
	}
}

SpanishRelationModel::SpanishRelationModel(const char *file_prefix)
{
	USE_PATTERNS = ParamReader::isParamTrue("use_relation_pattern_model");
	
	if (USE_PATTERNS) {
		_patternModel = _new PatternMatcherModel(file_prefix);
	} else {
		_vectorModel = _new VectorModel(file_prefix);
		_treeModel = _new TreeModel(file_prefix);
	}
}

int SpanishRelationModel::findBestRelationType(PotentialRelationInstance *instance) {

	if (USE_PATTERNS) {
		Symbol sym = _patternModel->findBestRelationType(instance);
		return RelationTypeSet::getTypeFromSymbol(sym);
	} else {
		int result = findBestRelationTypeBasic(instance);
		if (!RelationTypeSet::isNull(result))
			return result;

		if (USE_METONYMY)
			return findBestRelationTypeLeftMetonymy(instance);
		else return 0;

	}
}

int SpanishRelationModel::findBestRelationTypeBasic(PotentialRelationInstance *instance) {
	int v_type = _vectorModel->findBestRelationType(instance);
	int t_type = _treeModel->findBestRelationType(instance);

	// MODIFIED OR
	// if both null: return 0
	// if both non-null: return vector
	// if only one null, and it has a zero probability (model knew nothing), return other
	/*if (RelationTypeSet::isNull(v_type) && RelationTypeSet::isNull(t_type))
		return 0;
	else if (!RelationTypeSet::isNull(v_type) && !RelationTypeSet::isNull(t_type))
		return v_type;
	else if (RelationTypeSet::isNull(v_type) && _vectorModel->hasZeroProbability(instance, v_type))
		return t_type;
	else if (RelationTypeSet::isNull(t_type) && _treeModel->hasZeroProbability(instance, t_type))
		return v_type;
	else return 0;*/

	// OR
	if (RelationTypeSet::isNull(v_type) || RelationTypeSet::isNull(t_type))
		return 0;
	else return v_type;

	// AND
	/*if (RelationTypeSet::isNull(v_type) && RelationTypeSet::isNull(t_type))
		return 0;
	else if (!RelationTypeSet::isNull(v_type))
		return v_type;
	else return t_type;*/
}

int SpanishRelationModel::findBestRelationTypeLeftMetonymy(PotentialRelationInstance *instance) {
	int result = 0;
	EntityType leftEntityType(instance->getLeftEntityType());
	if (leftEntityType.matchesORG()) {
		instance->setLeftEntityType(EntityType::getFACType().getName());
		result = findBestRelationTypeBasic(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		result = findBestRelationTypeRightMetonymy(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		instance->setLeftEntityType(EntityType::getORGType().getName());
	} else if (leftEntityType.matchesFAC()) {
		instance->setLeftEntityType(EntityType::getORGType().getName());
		result = findBestRelationTypeBasic(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		result = findBestRelationTypeRightMetonymy(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		instance->setLeftEntityType(EntityType::getFACType().getName());
	}
	return 0;
}

int SpanishRelationModel::findBestRelationTypeRightMetonymy(PotentialRelationInstance *instance) {
	int result = 0;
	EntityType rightEntityType(instance->getRightEntityType());
	if (rightEntityType.matchesORG()) {
		instance->setRightEntityType(EntityType::getFACType().getName());
		result = findBestRelationTypeBasic(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		instance->setRightEntityType(EntityType::getORGType().getName());
	} else if (rightEntityType.matchesFAC()) {
		instance->setRightEntityType(EntityType::getORGType().getName());
		result = findBestRelationTypeBasic(instance);
		if (!RelationTypeSet::isNull(result))
			return result;
		instance->setRightEntityType(EntityType::getFACType().getName());
	}
	return 0;
}

