// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/SessionLogger.h"

Symbol PotentialRelationInstance::CONFUSED_SYM = Symbol(L":CONFUSED");
Symbol PotentialRelationInstance::ONE_PLACE = Symbol(L"ONE_PLACE");
Symbol PotentialRelationInstance::NONE_SYM = Symbol(L"NONE");
Symbol PotentialRelationInstance::REVERSED_SYM = Symbol(L"reversed");
Symbol PotentialRelationInstance::NULL_SYM = Symbol(L"NULL");
Symbol PotentialRelationInstance::MULTI_PLACE = Symbol(L"MULTI_PLACE");
Symbol PotentialRelationInstance::PARTITIVE_TOP = Symbol(L"partitive_top");
Symbol PotentialRelationInstance::PARTITIVE_BOTTOM = Symbol(L"partitive_bottom");
Symbol PotentialRelationInstance::SET_SYM = Symbol(L":SET");
Symbol PotentialRelationInstance::COMP_SYM = Symbol(L":COMP");

// 0 reltype 
// 1 predicate 
// 2 stemmedpredicate 
// 3 leftword 
// 4 rightword 
// 5 nestedword 
// 6 lefttype 
// 7 righttype 
// 8 leftcxion 
// 9 rightcxion
// 10 nestedcxion
// 11 reversed
// 12 predication_type

PotentialRelationInstance::PotentialRelationInstance() {
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		_ngram[i] = NULL_SYM;
	}
	_leftMention = MentionUID();
	_rightMention = MentionUID();
}

void PotentialRelationInstance::setFromTrainingNgram(Symbol *training_instance)
{
	setRelationType(training_instance[0]);
	setPredicate(training_instance[1]);
	setStemmedPredicate(training_instance[2]);
	setLeftHeadword(training_instance[3]); 
	setRightHeadword(training_instance[4]); 
	setNestedWord(training_instance[5]);
	setLeftEntityType(training_instance[6]);
	setRightEntityType(training_instance[7]);
	setLeftRole(training_instance[8]);
	setRightRole(training_instance[9]);
	setNestedRole(training_instance[10]);

	setReverse(NULL_SYM);	
	if (training_instance[11] == REVERSED_SYM) {
		int t = RelationTypeSet::getTypeFromSymbol(getRelationType());
		if (!RelationTypeSet::isSymmetric(t))
			setReverse(REVERSED_SYM);
	}

	setPredicationType(training_instance[12]);
}

void PotentialRelationInstance::setStandardInstance(RelationObservation *obs) {

	// set relation type to default (NONE)
	setRelationType(RelationConstants::NONE);

	// set mention indicies
	_leftMention = obs->getMention1()->getUID();
	_rightMention = obs->getMention2()->getUID();
	
	setLeftHeadword(obs->getMention1()->getNode()->getHeadWord());
	setRightHeadword(obs->getMention2()->getNode()->getHeadWord());
	setNestedWord(NULL_SYM); 
	setLeftEntityType(obs->getMention1()->getEntityType().getName());
	setRightEntityType(obs->getMention2()->getEntityType().getName());		
	setNestedRole(NULL_SYM); 
	setReverse(NULL_SYM); 

	if (obs->getPropLink()->isEmpty()) {
		setPredicate(NULL_SYM);
		setStemmedPredicate(NULL_SYM);
		setLeftRole(NULL_SYM);
		setRightRole(NULL_SYM);
		setPredicationType(MULTI_PLACE);
		return;
	}

	Proposition *prop = obs->getPropLink()->getTopProposition();
	
	// set predicate
	if (prop->getPredType() == Proposition::NOUN_PRED ||
		prop->getPredType() == Proposition::VERB_PRED ||
		prop->getPredType() == Proposition::COPULA_PRED ||
		prop->getPredType() == Proposition::MODIFIER_PRED ||
		prop->getPredType() == Proposition::POSS_PRED)
	{
		setPredicate(prop->getPredSymbol());
	}
	else if (prop->getPredType() == Proposition::SET_PRED) 
		setPredicate(SET_SYM);
	else if (prop->getPredType() == Proposition::COMP_PRED)
		setPredicate(COMP_SYM);
	else setPredicate(CONFUSED_SYM);

	// set stemmed predicate -- language specific
	setStemmedPredicate(RelationUtilities::get()->stemPredicate(prop->getPredSymbol(), 
		prop->getPredType()));

	// set everything else
	setLeftRole(obs->getPropLink()->getArg1Role());
	setRightRole(obs->getPropLink()->getArg2Role());
	if (obs->getPropLink()->getArg1Role() == Argument::REF_ROLE)
		setPredicationType(ONE_PLACE);
	else setPredicationType(MULTI_PLACE);

}
	

void PotentialRelationInstance::setStandardInstance(
	Argument *first, Argument *second, const Proposition *prop,
	const MentionSet *mentionSet) 
{

	// set relation type to default (NONE)
	setRelationType(RelationConstants::NONE);

	// set mention indicies
	_leftMention = first->getMention(mentionSet)->getUID();
	_rightMention = second->getMention(mentionSet)->getUID();
	
	// set predicate
	if (prop->getPredType() == Proposition::NOUN_PRED ||
		prop->getPredType() == Proposition::VERB_PRED ||
		prop->getPredType() == Proposition::COPULA_PRED ||
		prop->getPredType() == Proposition::MODIFIER_PRED ||
		prop->getPredType() == Proposition::POSS_PRED)
	{
		setPredicate(prop->getPredSymbol());
	}
	else if (prop->getPredType() == Proposition::SET_PRED) 
		setPredicate(SET_SYM);
	else if (prop->getPredType() == Proposition::COMP_PRED)
		setPredicate(COMP_SYM);
	else
	{
		setPredicate(CONFUSED_SYM);
	}

	// set stemmed predicate -- language specific
	setStemmedPredicate(RelationUtilities::get()->stemPredicate(prop->getPredSymbol(), 
		prop->getPredType()));

	// set everything else
	setLeftHeadword(first->getMention(mentionSet)->getNode()->getHeadWord());
	setRightHeadword(second->getMention(mentionSet)->getNode()->getHeadWord());
	setNestedWord(NULL_SYM); 
	setLeftEntityType(first->getMention(mentionSet)->getEntityType().getName());
	setRightEntityType(second->getMention(mentionSet)->getEntityType().getName());
	setLeftRole(first->getRoleSym());
	setRightRole(second->getRoleSym());
	setNestedRole(NULL_SYM); 
	setReverse(NULL_SYM); 
	if (first->getRoleSym() == Argument::REF_ROLE)
		setPredicationType(ONE_PLACE);
	else setPredicationType(MULTI_PLACE);
}

void PotentialRelationInstance::setPartitiveInstance(Symbol top_headword, Symbol bottom_headword, 
						  Symbol entity_type)
{
	setRelationType(RelationConstants::NONE);
	setPredicate(top_headword);
	setStemmedPredicate(RelationUtilities::get()->stemPredicate(top_headword, Proposition::NOUN_PRED));
	setPredicationType(ONE_PLACE);
	setLeftHeadword(top_headword); 
	setRightHeadword(bottom_headword); 
	setNestedWord(NULL_SYM);
	setLeftEntityType(entity_type);
	setRightEntityType(entity_type);
	setLeftRole(PARTITIVE_TOP);
	setRightRole(PARTITIVE_BOTTOM);
	setNestedRole(NULL_SYM);
	setReverse(NULL_SYM); 
}


void PotentialRelationInstance::setStandardNestedInstance(
	Argument *first, Argument *intermediate, Argument *second,
	const Proposition *outer_prop, const Proposition *inner_prop,
	const MentionSet *mentionSet)
{
	// set all the basics in regular function
	setStandardInstance(first, second, outer_prop, mentionSet);

	// get nested word from inner_prop
	Symbol nested_word = inner_prop->getPredSymbol(); 

	// VERB_PREDs shouldn't get here in English, but again, shouldn't hurt anyone
	if (inner_prop->getPredType() == Proposition::NOUN_PRED ||
		inner_prop->getPredType() == Proposition::MODIFIER_PRED ||
		inner_prop->getPredType() == Proposition::VERB_PRED) 
	{
		setNestedWord(RelationUtilities::get()->stemPredicate(nested_word, inner_prop->getPredType()));
	} else {
		setNestedWord(CONFUSED_SYM);
	}
	
	setNestedRole(intermediate->getRoleSym());
}

void PotentialRelationInstance::setStandardListInstance(Argument *first, 
		Argument *second, bool listIsFirst, const Mention *member, const Proposition *prop, 
		const MentionSet *mentionSet)
{
	setStandardInstance(first, second, prop, mentionSet);
	if (listIsFirst) {
		_leftMention = member->getUID();
		setLeftHeadword(member->getNode()->getHeadWord());
		setLeftEntityType(member->getEntityType().getName());
	} else {
		_rightMention = member->getUID();
		setRightHeadword(member->getNode()->getHeadWord());
		setRightEntityType(member->getEntityType().getName());
	}
}

Symbol PotentialRelationInstance::getRelationType() { return _ngram[0]; }
Symbol PotentialRelationInstance::getPredicate() { return _ngram[1]; }
Symbol PotentialRelationInstance::getStemmedPredicate() { return _ngram[2]; }
Symbol PotentialRelationInstance::getLeftHeadword() { return _ngram[3]; }
Symbol PotentialRelationInstance::getRightHeadword() { return _ngram[4]; }
Symbol PotentialRelationInstance::getNestedWord() { return _ngram[5]; }
Symbol PotentialRelationInstance::getLeftEntityType() { return _ngram[6]; }
Symbol PotentialRelationInstance::getRightEntityType() { return _ngram[7]; }
Symbol PotentialRelationInstance::getLeftRole() { return _ngram[8]; }
Symbol PotentialRelationInstance::getRightRole() { return _ngram[9]; }	
Symbol PotentialRelationInstance::getNestedRole() { return _ngram[10]; }	
Symbol PotentialRelationInstance::getReverse() { return _ngram[11]; }
Symbol PotentialRelationInstance::getPredicationType() { return _ngram[12]; }	

void PotentialRelationInstance::setRelationType(Symbol sym) { _ngram[0] = sym; }
void PotentialRelationInstance::setPredicate(Symbol sym) { _ngram[1] = sym; }
void PotentialRelationInstance::setStemmedPredicate(Symbol sym) { _ngram[2] = sym; }
void PotentialRelationInstance::setLeftHeadword(Symbol sym) { _ngram[3] = sym; }
void PotentialRelationInstance::setRightHeadword(Symbol sym) { _ngram[4] = sym; }
void PotentialRelationInstance::setNestedWord(Symbol sym) { _ngram[5] = sym; }
void PotentialRelationInstance::setLeftEntityType(Symbol sym) { _ngram[6] = sym; }
void PotentialRelationInstance::setRightEntityType(Symbol sym) { _ngram[7] = sym; }
void PotentialRelationInstance::setLeftRole(Symbol sym) { _ngram[8] = sym; }
void PotentialRelationInstance::setRightRole(Symbol sym) { _ngram[9] = sym; }	
void PotentialRelationInstance::setNestedRole(Symbol sym) { _ngram[10] = sym; }	
void PotentialRelationInstance::setReverse(Symbol sym) { _ngram[11] = sym; }	
void PotentialRelationInstance::setPredicationType(Symbol sym) { _ngram[12] = sym; }	

MentionUID PotentialRelationInstance::getLeftMention() { return _leftMention; }
MentionUID PotentialRelationInstance::getRightMention() { return _rightMention; }

bool PotentialRelationInstance::isNested() { return getNestedRole() != NULL_SYM; }
bool PotentialRelationInstance::isOnePlacePredicate() {
	return getPredicationType() == ONE_PLACE;
}
bool PotentialRelationInstance::isMultiPlacePredicate() {
	return getPredicationType() == MULTI_PLACE;
}
bool PotentialRelationInstance::isReversed() {
	return getReverse() == REVERSED_SYM;
}
void PotentialRelationInstance::setReverse(bool reverse) {
	if (reverse)
		setReverse(REVERSED_SYM);
	else setReverse(NULL_SYM);
}

std::wstring PotentialRelationInstance::toString() {
	std::wstring result = L"";
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_string();
		result += L" ";
	}
	return result;
}
std::string PotentialRelationInstance::toDebugString() {
	std::string result = "";
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_debug_string();
		result += " ";
	}
	return result;
}
