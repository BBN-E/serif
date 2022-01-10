// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_PotentialRelationInstance.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SessionLogger.h"

// 0 last hanzi of predicate
// 1 left mention type
// 2 right mention type
// 3 left metonymy
// 4 right metonymy

ChinesePotentialRelationInstance::ChinesePotentialRelationInstance() {
	for (int i = 0; i < CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		_ngram[i] = NULL_SYM;
	}
}

void ChinesePotentialRelationInstance::setFromTrainingNgram(Symbol *training_instance)
{
	// call base class version
	PotentialRelationInstance::setFromTrainingNgram(training_instance);

	setLastHanziOfPredicate(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE]);
	setLeftMentionType(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1]);
	setRightMentionType(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2]);
	setLeftMetonymy(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+3]);
	setRightMetonymy(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+4]);
}

Symbol* ChinesePotentialRelationInstance::getTrainingNgram() {
	
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		_ngram[i] = PotentialRelationInstance::_ngram[i];
	}
	return _ngram;
}

void ChinesePotentialRelationInstance::setStandardInstance(
	Argument *first, Argument *second, const Proposition *prop,
	const MentionSet *mentionSet) 
{
	// First, set the members of the generic PotentialRelationInstance

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
		prop->getPredType() == Proposition::POSS_PRED ||
		prop->getPredType() == Proposition::NAME_PRED)
	{
		setPredicate(prop->getPredSymbol());
	}
	else if (prop->getPredType() == Proposition::SET_PRED) 
		setPredicate(SET_SYM);
	else if (prop->getPredType() == Proposition::COMP_PRED)
		setPredicate(COMP_SYM);
	else
	{
		SessionLogger::warn("potential_relation_instance") << "unknown predicate type in "
			<< "ChinesePotentialRelationInstance::setStandardInstance(): "
			<< prop->getPredTypeString(prop->getPredType());
		setPredicate(CONFUSED_SYM);
	}

	// set stemmed predicate -- language specific
	setStemmedPredicate(RelationUtilities::get()->stemPredicate(prop->getPredSymbol(), 
		prop->getPredType()));

	
	
	// set head words - make an exception for APPO - we want the head word to be the head word of the DESC
	if (first->getMention(mentionSet)->getMentionType() == Mention::APPO) 
		setLeftHeadword(first->getMention(mentionSet)->getChild()->getNode()->getHeadWord());
	else
		setLeftHeadword(first->getMention(mentionSet)->getNode()->getHeadWord());
	if (second->getMention(mentionSet)->getMentionType() == Mention::APPO) 
		setRightHeadword(second->getMention(mentionSet)->getChild()->getNode()->getHeadWord());
	else
		setRightHeadword(second->getMention(mentionSet)->getNode()->getHeadWord());
	// set everything else
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

	// Now set the Chinese specific members

	const wchar_t *pred = getPredicate().to_string();
	// unless the predicate is ':SET' or ':COMP'
	if (pred[0] != ':') {
		wchar_t str[2];
		size_t len = wcslen(pred);
		wcsncpy(str, pred + len - 1, 2);
		setLastHanziOfPredicate(Symbol(str));
	} 
	else 
		setLastHanziOfPredicate(getPredicate());

	setLeftMentionType(first->getMention(mentionSet)->getMentionType());
	setRightMentionType(second->getMention(mentionSet)->getMentionType());
	if (first->getMention(mentionSet)->hasIntendedType())
		setLeftMetonymy(first->getMention(mentionSet)->getIntendedType().getName());
	else if (first->getMention(mentionSet)->hasRoleType())
		setLeftMetonymy(first->getMention(mentionSet)->getRoleType().getName());
	if (second->getMention(mentionSet)->hasIntendedType())
		setRightMetonymy(second->getMention(mentionSet)->getIntendedType().getName());
	else if (second->getMention(mentionSet)->hasRoleType())
		setRightMetonymy(second->getMention(mentionSet)->getRoleType().getName());

}


void ChinesePotentialRelationInstance::setStandardNestedInstance(
	Argument *first, Argument *intermediate, Argument *second,
	const Proposition *outer_prop, const Proposition *inner_prop,
	const MentionSet *mentionSet)
{
	// set all the basics in regular function
	setStandardInstance(first, second, outer_prop, mentionSet);

	// get nested word from inner_prop
	Symbol nested_word = inner_prop->getPredSymbol(); 

	// get nested word from inner_prop
	if (inner_prop->getPredType() == Proposition::NOUN_PRED ||
		inner_prop->getPredType() == Proposition::MODIFIER_PRED ||
		inner_prop->getPredType() == Proposition::VERB_PRED ||
		inner_prop->getPredType() == Proposition::COPULA_PRED) 
	{
		setNestedWord(RelationUtilities::get()->stemPredicate(nested_word, inner_prop->getPredType()));
	}
	else if (inner_prop->getPredType() == Proposition::SET_PRED)  
		setNestedWord(SET_SYM);
	else if (inner_prop->getPredType() == Proposition::COMP_PRED) 
		setNestedWord(COMP_SYM);
	else {
		SessionLogger::warn("potential_relation_instance") 
			<< "unknown predicate type in "
			<< "ChinesePotentialRelationInstance::setStandardNestedInstance(): "
			<< inner_prop->getPredTypeString(inner_prop->getPredType());
		setNestedWord(CONFUSED_SYM);
	}
	
	setNestedRole(intermediate->getRoleSym());
}

void ChinesePotentialRelationInstance::setStandardListInstance(Argument *first, 
		Argument *second, bool listIsFirst, const Mention *member, const Proposition *prop, 
		const MentionSet *mentionSet)
{	
	setStandardInstance(first, second, prop, mentionSet);

	if (listIsFirst) {
		_leftMention = member->getUID();
		if (member->getMentionType() == Mention::APPO) 
			setLeftHeadword(member->getChild()->getNode()->getHeadWord());
		else
			setLeftHeadword(member->getNode()->getHeadWord());
		setLeftEntityType(member->getEntityType().getName());
		setLeftMentionType(member->getMentionType());
		if (member->hasIntendedType())
			setLeftMetonymy(member->getIntendedType().getName());
		else if (member->hasRoleType())
			setLeftMetonymy(member->getRoleType().getName());
	} else {
		_rightMention = member->getUID();
		if (member->getMentionType() == Mention::APPO) 
			setRightHeadword(member->getChild()->getNode()->getHeadWord());
		else
			setRightHeadword(member->getNode()->getHeadWord());
		setRightEntityType(member->getEntityType().getName());
		setRightMentionType(member->getMentionType());
		setRightMetonymy(member->getIntendedType().getName());
		if (member->hasIntendedType())
			setRightMetonymy(member->getIntendedType().getName());
		else if (member->hasRoleType())
			setRightMetonymy(member->getRoleType().getName());
	}
}

Symbol ChinesePotentialRelationInstance::getLastHanziOfPredicate() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE]; }
Symbol ChinesePotentialRelationInstance::getLeftMentionType() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1]; }
Symbol ChinesePotentialRelationInstance::getRightMentionType() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2]; }
Symbol ChinesePotentialRelationInstance::getLeftMetonymy() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+3]; }
Symbol ChinesePotentialRelationInstance::getRightMetonymy() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+4]; }	

void ChinesePotentialRelationInstance::setLastHanziOfPredicate(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE] = sym; }
void ChinesePotentialRelationInstance::setLeftMentionType(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1] = sym; }
void ChinesePotentialRelationInstance::setRightMentionType(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2] = sym; }
void ChinesePotentialRelationInstance::setLeftMetonymy(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+3] = sym; }
void ChinesePotentialRelationInstance::setRightMetonymy(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+4] = sym; }

void ChinesePotentialRelationInstance::setLeftMentionType(Mention::Type type) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1] = convertMentionType(type); }
void ChinesePotentialRelationInstance::setRightMentionType(Mention::Type type) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2] = convertMentionType(type); }

std::wstring ChinesePotentialRelationInstance::toString() {
	std::wstring result = PotentialRelationInstance::toString();
	for (int i = POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i < CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_string();
		result += L" ";
	}
	return result;
}
std::string ChinesePotentialRelationInstance::toDebugString() {
	std::string result = PotentialRelationInstance::toDebugString();
	for (int i = POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i < CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_debug_string();
		result += " ";
	}
	return result;
}

Symbol ChinesePotentialRelationInstance::convertMentionType(Mention::Type type) {
	if (type == Mention::NONE) 
		return Symbol(L"NONE");
	else if (type == Mention::NAME)
		return Symbol(L"NAME");
	else if (type == Mention::PRON)
		return Symbol(L"PRON");
	else if (type == Mention::DESC)
		return Symbol(L"DESC");
	else if (type == Mention::PART)
		return Symbol(L"PART");
	else if (type == Mention::APPO)
		return Symbol(L"APPO");
	else if (type == Mention::LIST)
		return Symbol(L"LIST");
	return Symbol();
}
