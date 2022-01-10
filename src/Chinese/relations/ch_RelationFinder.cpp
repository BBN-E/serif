// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_RelationFinder.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/RelMentionSet.h"
#include "Chinese/relations/ch_MaxEntRelationFinder.h"
#include "Chinese/relations/discmodel/ch_P1RelationFinder.h"
#include "Chinese/relations/ch_OldMaxEntRelationFinder.h"
#include "Chinese/relations/ch_ComboRelationFinder.h"


ChineseRelationFinder::ChineseRelationFinder() :
	_maxEntRelationFinder(0), _p1RelationFinder(0), 
	_oldMaxEntRelationFinder(0), _comboRelationFinder(0)
{
	std::string model_type = ParamReader::getRequiredParam("relation_model_type");
	if (model_type == "MAXENT")
		mode = MAXENT;
	else if (model_type == "P1")
		mode = P1;
	else if (model_type == "COMBO")
		mode = COMBO;
	else if (model_type == "OLD_MAXENT")
		mode = OLD_MAXENT;
	else if (model_type == "NONE")
		mode = NONE;
	else {
		std::string error = model_type + "not a valid relation model type: use NONE, P1, MAXENT, OLD_MAXENT or COMBO";
		throw UnexpectedInputException("ChineseRelationFinder::ChineseRelationFinder()", error.c_str());
	}

	if (mode == P1)
		_p1RelationFinder = _new ChineseP1RelationFinder();
	else if (mode == OLD_MAXENT)
		_oldMaxEntRelationFinder = _new OldMaxEntRelationFinder();
	else if (mode == MAXENT)
		_maxEntRelationFinder = _new ChineseMaxEntRelationFinder();
	else if (mode == COMBO)
		_comboRelationFinder = _new ChineseComboRelationFinder();
}

ChineseRelationFinder::~ChineseRelationFinder() {
	delete _p1RelationFinder;
	delete _maxEntRelationFinder;
	delete _oldMaxEntRelationFinder;
	delete _comboRelationFinder;
}

void ChineseRelationFinder::cleanup() {
	_p1RelationFinder->cleanup();
	_maxEntRelationFinder->cleanup();
	_oldMaxEntRelationFinder->cleanup();
	_comboRelationFinder->cleanup();
}


void ChineseRelationFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {
	if (mode == P1)
		return _p1RelationFinder->resetForNewSentence();
	else if (mode == OLD_MAXENT)
		return _oldMaxEntRelationFinder->resetForNewSentence();
	else if (mode == MAXENT)
		return _maxEntRelationFinder->resetForNewSentence();
	else if (mode == COMBO)
		return _comboRelationFinder->resetForNewSentence();
}

RelMentionSet *ChineseRelationFinder::getRelationTheory(EntitySet *entitySet,
							   const Parse *parse,
		                       MentionSet *mentionSet,
							   ValueMentionSet *valueMentionSet, 
		                       PropositionSet *propSet,
							   const Parse *secondaryParse,
							   const PropTreeLinks* ptLinks)
{
	if (mode == P1)
		return _p1RelationFinder->getRelationTheory(entitySet, parse, mentionSet, valueMentionSet, propSet);
	else if (mode == OLD_MAXENT)
		return _oldMaxEntRelationFinder->getRelationTheory(parse, mentionSet, propSet);
	else if (mode == MAXENT)
		return _maxEntRelationFinder->getRelationTheory(entitySet, parse, mentionSet, valueMentionSet, propSet);
	else if (mode == COMBO)
		return _comboRelationFinder->getRelationTheory(entitySet, parse, mentionSet, valueMentionSet, propSet);
	else
		return _new RelMentionSet();

}

void ChineseRelationFinder::allowMentionSetChanges() {
	if (mode == COMBO)
		_comboRelationFinder->allowMentionSetChanges();
}
void ChineseRelationFinder::disallowMentionSetChanges() {
	if (mode == COMBO)
		_comboRelationFinder->disallowMentionSetChanges();
}

