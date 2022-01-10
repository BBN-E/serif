// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/relations/es_OldRelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Spanish/relations/es_RelationUtilities.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Spanish/relations/es_RelationModel.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Spanish/relations/es_SpecialRelationCases.h"
#include "Generic/rawrelations/RawRelationFinder.h"
#include "Spanish/common/es_WordConstants.h"
#include "Generic/events/EventUtilities.h"

#include "Generic/CASerif/correctanswers/CorrectMention.h"

UTF8OutputStream OldRelationFinder::_debugStream;
bool OldRelationFinder::DEBUG = false;
int OldRelationFinder::_relationFinderType = OldRelationFinder::NONE;

static Symbol number_symbol(L"<number>");
static Symbol in_symbol(L"in");

OldRelationFinder::OldRelationFinder() {
	RelationTypeSet::initialize();

	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

	std::string debug_buffer = ParamReader::getParam("relation_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}
	_instance = _new PotentialRelationInstance();
	_specialRelationCases = _new SpecialRelationCases();

	std::string model_prefix = ParamReader::getRequiredParam("relation_model");
	_model = RelationModel::build(model_prefix.c_str());

	std::string parameter = ParamReader::getRequiredParam("relation_finder_type");
	if (parameter == "EELD")
		_relationFinderType = EELD;
	else if (parameter == "ACE")
		_relationFinderType = ACE;
	else if (parameter == "ITEA")
		_relationFinderType = ITEA;
	else {
		std::string error = parameter + " is not a valid relation finder type: use ACE, EELD, or ITEA";
		throw UnexpectedInputException("OldRelationFinder::OldRelationFinder()", error.c_str());
	}

	_enable_raw_relations = ParamReader::isParamTrue("enable_raw_relations");
	_look_inside_non_core_types = ParamReader::isParamTrue("look_inside_non_core_types");
	_allow_type_coercing = false;
}

OldRelationFinder::~OldRelationFinder() {
	delete _instance;
	delete _specialRelationCases;
	delete _model;
}


void OldRelationFinder::resetForNewSentence() {

	_n_relations = 0;

}

RelMentionSet *OldRelationFinder::getRelationTheory(const Parse *parse,
											   MentionSet *mentionSet,
											   const PropositionSet *propSet)
{
	_n_relations = 0;
	_currentMentionSet = mentionSet;
	_currentSentenceIndex = mentionSet->getSentenceNumber();

	std::vector<bool> is_false_or_hypothetical = RelationUtilities::get()->identifyFalseOrHypotheticalProps(propSet, mentionSet);
	collectDefinitions(propSet);

	if (DEBUG) {
		if (_currentSentenceIndex == 0) {
			_debugStream << L"*** NEW DOCUMENT ***\n";
		}
		_debugStream << L"*** NEW SENTENCE ***\n";
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			propSet->getProposition(i)->dump(_debugStream, 0);
			if (is_false_or_hypothetical[propSet->getProposition(i)->getIndex()])
				_debugStream << L" -- UNTRUTH";
			_debugStream << L"\n";
		}
		_debugStream << "TEXT: " << parse->getRoot()->toTextString() << L"\n";
		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *ment = mentionSet->getMention(j);
			if (ment->getEntityType().isRecognized())
				_debugStream << ment->getNode()->toTextString() << ": " << ment->getEntityType().getName().to_string() << "\n";
		}
		_debugStream << parse->getRoot()->toPrettyParse(0) << L"\n";

/*if (_use_correct_answers) {

		for (int k = 0; k < mentionSet->getNMentions(); k++) {
			Mention *ment1 = mentionSet->getMention(k);
			if (!ment1->getEntityType().isRecognized())
				continue;
			CorrectMention *firstCM = currentCorrectDocument->getCorrectMentionFromMentionID(ment1->getUID());
			if (firstCM == NULL)
				continue;

			if (ment1->getNode()->getHead() != 0 && ment1->getNode()->getHead()->hasMention()) {
				Mention *head = mentionSet->getMentionByNode(ment1->getNode()->getHead());
				if (head->getUID() != ment1->getUID()) {
					CorrectMention *headCM
						= currentCorrectDocument->getCorrectMentionFromMentionID(head->getUID());
					if (headCM != NULL && (headCM == firstCM ||
						(currentCorrectDocument->getCorrectEntityFromCorrectMention(headCM) ==
						 currentCorrectDocument->getCorrectEntityFromCorrectMention(firstCM))))
						continue;
				}
			}

			for (int m = k+1; m < mentionSet->getNMentions(); m++) {
				Mention *ment2 = mentionSet->getMention(m);
				if (!ment2->getEntityType().isRecognized())
					continue;

				CorrectMention *secondCM = currentCorrectDocument->getCorrectMentionFromMentionID(ment2->getUID());
				if (secondCM == NULL)
					continue;

				if (ment2->getNode()->getHead() != 0 && ment2->getNode()->getHead()->hasMention()) {
					Mention *head = mentionSet->getMentionByNode(ment2->getNode()->getHead());
					if (head->getUID() != ment2->getUID()) {
						CorrectMention *headCM
							= currentCorrectDocument->getCorrectMentionFromMentionID(head->getUID());
						if (headCM != NULL && (headCM == secondCM ||
						(currentCorrectDocument->getCorrectEntityFromCorrectMention(headCM) ==
						 currentCorrectDocument->getCorrectEntityFromCorrectMention(secondCM))))
							continue;
					}
				}

				_debugStream << L"\nCONSIDERING: ";
				_debugStream << ment1->getNode()->toTextString();
				_debugStream << L" AND ";
				_debugStream << ment2->getNode()->toTextString();
				_debugStream << L"\n";
				_debugStream << firstCM->annotationID.to_string() << L" " << secondCM->annotationID.to_string() << L"\n";
				const SynNode *s1 = ment1->getNode()->getHeadPreterm();
				if (s1->getParent() != 0 && s1->getParent()->getTag() == SpanishSTags::NPP)
					s1 = s1->getParent();
				const SynNode *s2 = ment2->getNode()->getHeadPreterm();
				if (s2->getParent() != 0 && s2->getParent()->getTag() == SpanishSTags::NPP)
					s2 = s2->getParent();
				_debugStream << L"FIRST CM TOKENS: " << s1->getStartToken() << L" ";
				_debugStream << s1->getEndToken() << L"\n";
				_debugStream << L"SECOND CM TOKENS: " << s2->getStartToken() << L" ";
				_debugStream << s2->getEndToken() << L"\n";
			}
		}
  }*/
	}

	addPartitiveRelations();

	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		if (is_false_or_hypothetical[prop->getIndex()])
			continue;

		int n_previous_relations = _n_relations;

		if (prop->getPredType() == Proposition::COPULA_PRED) {
			classifyCopulaProposition(prop);
		} else if (prop->getPredType() == Proposition::LOC_PRED) {
			classifyLocationProposition(prop);
		} else if (prop->getPredType() == Proposition::SET_PRED) {
			classifySetProposition(prop);
		} else if (prop->getPredType() == Proposition::VERB_PRED ||
				   prop->getPredType() == Proposition::POSS_PRED ||
				   prop->getPredType() == Proposition::MODIFIER_PRED ||
				   prop->getPredType() == Proposition::NOUN_PRED) {
			classifyProposition(prop);
		}

		// SRS: "raw" (propositional) relations
		if (_enable_raw_relations &&
			_n_relations == n_previous_relations)
		{
			addRawRelations(prop, mentionSet);
		}
	}

	RelMentionSet *result = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		result->takeRelMention(_relations[j]);
		_relations[j] = 0;
	}

	if (DEBUG) _debugStream << L"\nSCORE:" << result->getScore() << L"\n";
	return result;
}

void OldRelationFinder::classifyLocationProposition(Proposition *prop) {
	if (prop->getNArgs() != 2) {
		if (DEBUG)
			_debugStream << L"\nLocation proposition with " << prop->getNArgs() << " arguments!\n";
		return;
	}
	const Mention *city = prop->getArg(0)->getMention(_currentMentionSet);
	const Mention *state = prop->getArg(1)->getMention(_currentMentionSet);

	if (!prop->getPredSymbol().is_null())
		return;

	int type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getGPEPartType());
	if (type != RelationTypeSet::INVALID_TYPE)
		addRelation(city, state, type, 1);
}


void OldRelationFinder::classifyCopulaProposition(Proposition *prop) {

	const Mention *subject = 0;
	const Mention *object = 0;

	for (int i = 0; i < prop->getNArgs(); i++) {
		Argument *arg = prop->getArg(i);
		if (arg->getType() == Argument::MENTION_ARG) {
			if (arg->getRoleSym() == Argument::SUB_ROLE) {
				subject = arg->getMention(_currentMentionSet);
			} else if (arg->getRoleSym() == Argument::OBJ_ROLE) {
				object = arg->getMention(_currentMentionSet);
			}
		}
	}

	if (subject == 0 || object == 0)
		return;

	int type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getGroupMembers());
	if (type != RelationTypeSet::INVALID_TYPE) {
		if (subject->getMentionType() == Mention::LIST) {
			if (isGroupMention(object)) {
				const Mention *iter = subject->getChild();
				while (iter != 0) {
					if (iter->getEntityType().isRecognized())
						addRelation(iter, object, type, 1);
					iter = iter->getNext();
				}
			}
		} else if (object->getMentionType() == Mention::LIST) {
			if (isGroupMention(subject)) {
				const Mention *iter = object->getChild();
				while (iter != 0) {
					if (iter->getEntityType().isRecognized())
						addRelation(iter, subject, type, 1);
					iter = iter->getNext();
				}
			}
		}
	}

	classifyProposition(prop);
}

bool OldRelationFinder::isGroupMention(const Mention* mention) {
	Symbol headword = mention->getNode()->getHeadWord();
	if (headword == SpanishWordConstants::THEY ||
		headword == SpanishWordConstants::THEM ||
		headword == SpanishWordConstants::WE ||
		headword == SpanishWordConstants::THESE ||
		headword == SpanishWordConstants::THOSE)
		return true;
	Symbol headtag = mention->getNode()->getHeadPreterm()->getTag();
	if (headtag == SpanishSTags::NNPS ||
		headtag == SpanishSTags::NNS)
		return true;
	return false;
}

void OldRelationFinder::classifySetProposition(Proposition *prop) {

	// bnews sign-offs
	if (prop->getNArgs() >= 4 &&
		prop->getArg(0)->getType() == Argument::MENTION_ARG &&
		prop->getArg(1)->getType() == Argument::MENTION_ARG &&
		prop->getArg(2)->getType() == Argument::MENTION_ARG &&
		prop->getArg(3)->getType() == Argument::MENTION_ARG) {
		const Mention *person = prop->getArg(1)->getMention(_currentMentionSet);
		const Mention *organization = prop->getArg(2)->getMention(_currentMentionSet);
		const Mention *location = prop->getArg(3)->getMention(_currentMentionSet);
		if (person->getEntityType().matchesPER() &&
			organization->getEntityType().matchesORG() &&
			(location->getEntityType().matchesGPE() ||
			location->getEntityType().matchesFAC() ||
			location->getEntityType().matchesLOC()))
		{
			int type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getEmployeesType());
			if (type != RelationTypeSet::INVALID_TYPE)
				addRelation(person, organization, type, 1);
			type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getLocatedType());
			if (type != RelationTypeSet::INVALID_TYPE)
				addRelation(person, location, type, 1);
			if (location->getEntityType().matchesGPE() &&
				prop->getNArgs() >= 5 &&
				prop->getArg(4)->getType() == Argument::MENTION_ARG)
			{
				const Mention *nextlocation = prop->getArg(4)->getMention(_currentMentionSet);
				if (nextlocation->getEntityType().matchesGPE()) {
					type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getGPEPartType());
					if (type != RelationTypeSet::INVALID_TYPE)
						addRelation(location, nextlocation, type, 1);
				}
			}
		}
	}

	// refine me later...
	if (prop->getNArgs() == 3 &&
		prop->getArg(0)->getType() == Argument::MENTION_ARG &&
		prop->getArg(1)->getType() == Argument::MENTION_ARG &&
		prop->getArg(2)->getType() == Argument::MENTION_ARG) {
		const SynNode *setNode = prop->getArg(0)->getMention(_currentMentionSet)->getNode();
		if (setNode->getParent() == 0 ||
			(setNode->getParent()->getParent() == 0 &&
			 setNode->getParent()->getNChildren() == 2 &&
			 setNode->getChild(1)->getHeadWord() == Symbol(L".")))
		{
			const Mention *person = prop->getArg(1)->getMention(_currentMentionSet);
			const Mention *organization = prop->getArg(2)->getMention(_currentMentionSet);
			if (person->getEntityType().matchesPER() &&
				organization->getEntityType().matchesORG() &&
				person->getMentionType() == Mention::NAME &&
				organization->getMentionType() == Mention::NAME)
			{
				int type
					= RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getEmployeesType());
				if (type != RelationTypeSet::INVALID_TYPE)
					addRelation(person, organization, type, 1);
			}
		}
	}
}

void OldRelationFinder::classifyProposition(Proposition *prop) {

	bool prop_is_definition_of_EDT_entity = false;
	if (prop->getPredType() == Proposition::NOUN_PRED &&
		prop->getArg(0)->getType() == Argument::MENTION_ARG &&
		prop->getArg(0)->getMention(_currentMentionSet)->getEntityType().isRecognized())
	{	prop_is_definition_of_EDT_entity = true; }

	Argument *whereArg = 0;
	if (_propTakesPlaceAt[prop->getIndex()] != 0) {
		whereArg = _new Argument();
		whereArg->populateWithMention(in_symbol,
			_propTakesPlaceAt[prop->getIndex()]->getMentionIndex());
	}

	// BASIC IDEA:
	//
	// Call classifyArgumentPair on every pair of arguments within
	// a proposition.
	//
	// For now, if we have the construction "the LOC, where X happened",
	// we consider it as if in:LOC were an argument to the proposition
	// "X happened". So, for example:
	//
	// "the place where he was killed"
	// p1 = place<noun>(<ref>:e1, where:p2)
	// p2 = killed<verb>(<obj>:e2)
	//  -->
	// argument pair: <obj>:e2 and in:e1

	for (int i = 0; i < prop->getNArgs(); i++) {
		if (prop->getArg(i)->getType() != Argument::MENTION_ARG)
			continue;

		// we don't nest through EDT entities (e.g. _his_ house in _Limassol_)
		if (prop_is_definition_of_EDT_entity &&	i > 0) {
			delete whereArg;
			return;
		}

		for (int j = i + 1; j < prop->getNArgs(); j++) {
			if (prop->getArg(j)->getType() != Argument::MENTION_ARG)
				continue;
			classifyArgumentPair(prop->getArg(i), prop->getArg(j), prop);
		}

		if (whereArg != 0) {
			classifyArgumentPair(prop->getArg(i), whereArg, prop);
		}
	}

	delete whereArg;

}

void OldRelationFinder::classifyArgumentPair(Argument *leftArg, Argument *rightArg,
										 Proposition *prop)
{
	const Mention* left = leftArg->getMention(_currentMentionSet);
	const Mention* right = rightArg->getMention(_currentMentionSet);

	// if leftArg and rightArg are both EDT entities, define a
	// PotentialRelationInstance over them and classify
	//
	// if one is and one isn't, but the non-EDT entity is a list,
	// go to FindListInstances
	//
	// if one is and one isn't, and the non-EDT entity isn't a list,
	// go to findNestedInstances
	//
	// if neither are, return.

	if (left->getEntityType().isRecognized() &&
		right->getEntityType().isRecognized()) {
		_instance->setStandardInstance(leftArg, rightArg, prop, _currentMentionSet);
		classifyInstance(left, right);

		if (_look_inside_non_core_types &&
			right->getEntityType().canBeRelArg())
		{
			findNestedInstances(leftArg, rightArg, prop);
		}
	} else if (left->getEntityType().isRecognized() &&
		       right->getMentionType() == Mention::LIST) {
		findListInstances(leftArg, rightArg, prop, false);
	} else if (right->getEntityType().isRecognized() &&
		       left->getMentionType() == Mention::LIST) {
		findListInstances(leftArg, rightArg, prop, true);
	} else if (left->getEntityType().isRecognized()) {
		if (!findWithCoercedType(leftArg, rightArg, prop))
			findNestedInstances(leftArg, rightArg, prop);
	} else if (right->getEntityType().isRecognized()) {
		// LB: we don't nest on this side (for now)
		findWithCoercedType(leftArg, rightArg, prop);
	}
}


/** this function deals specifically with lists of type OTH
 *  with children of only one EDT type
 *
 *  for example: in "a boxer and an emigre", we don't get "a boxer" as
 *  a PER, so the list is of type OTH. Same with "Bob, 52," (annoying but true)
 *
 */
void OldRelationFinder::findListInstances(Argument *left, Argument *right,
										   Proposition *prop, bool leftIsList)
{
	const Mention *iter;
	if (leftIsList)
		iter = left->getMention(_currentMentionSet)->getChild();
	else iter = right->getMention(_currentMentionSet)->getChild();

	// EELD has looser handling of lists -- ACE doesn't go through mismatched lists
	if (_relationFinderType == ACE) {
		EntityType etype = iter->getEntityType();
		const Mention *temp_iter = iter->getNext();
		while (temp_iter != 0) {
			if (temp_iter->getEntityType() != etype &&
				etype.isRecognized() &&
				temp_iter->getEntityType().isRecognized())
			{
				if (DEBUG) {
					_debugStream << "MISMATCHED SET: ";
					if (leftIsList)
						_debugStream << left->getMention(_currentMentionSet)->getNode()->toTextString();
					else _debugStream << right->getMention(_currentMentionSet)->getNode()->toTextString();
					_debugStream << L"\n";
				}
				return;
			}
			temp_iter = temp_iter->getNext();
		}
	}

	if (_relationFinderType == ITEA && !leftIsList) {
		Mention *stackedOrgs[10];
		int n_orgs = RelationUtilities::get()->getOrgStack(right->getMention(_currentMentionSet),
			stackedOrgs, 10);
		if (n_orgs != 0) {
			_instance->setStandardListInstance(left, right, leftIsList,
				iter, prop, _currentMentionSet);
			int type = _model->findBestRelationType(_instance);
			//if (type == RelationTypeSet::getTypeFromSymbol(
			//	SpecialRelationCases::getSubordinateType()))
			//{
			int subordinate_type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getSubordinateType());
			addRelation(left->getMention(_currentMentionSet), stackedOrgs[0], type, 1.0);
			for (int i = 0; i < n_orgs - 1; i++) {
				addRelation(stackedOrgs[i], stackedOrgs[i+1], subordinate_type, 1.0);
			}
			return;
			//}
		}
	}

	while (iter != 0) {
		if (iter->getEntityType().isRecognized()) {
			_instance->setStandardListInstance(left, right, leftIsList,
				iter, prop, _currentMentionSet);
			if (leftIsList)
				classifyInstance(iter, right->getMention(_currentMentionSet));
			else classifyInstance(left->getMention(_currentMentionSet), iter);
		}
		iter = iter->getNext();
	}

}


void OldRelationFinder::findNestedInstances(Argument *first, Argument *intermediate,
										 Proposition *prop)
{
	if (first->getRoleSym() != Argument::SUB_ROLE &&
		first->getRoleSym() != Argument::OBJ_ROLE &&
		first->getRoleSym() != Argument::REF_ROLE)
		return;

	int inner_term_index = intermediate->getMentionIndex();
	Proposition *inner_term = _definitions[inner_term_index];
	if (inner_term == 0)
		return;

	for (int i = 1; i < inner_term->getNArgs(); i++) {
		if (inner_term->getArg(i)->getType() != Argument::MENTION_ARG)
			continue;
		if (!inner_term->getArg(i)->getMention(_currentMentionSet)->getEntityType().isRecognized())
			continue;
		_instance->setStandardNestedInstance(first, intermediate, inner_term->getArg(i),
			prop, inner_term, _currentMentionSet);
		classifyInstance(first->getMention(_currentMentionSet),
			inner_term->getArg(i)->getMention(_currentMentionSet));
	}
}

bool OldRelationFinder::findWithCoercedType(Argument *leftArg, Argument *rightArg, Proposition *prop)
{
	const Mention* left = leftArg->getMention(_currentMentionSet);
	const Mention* right = rightArg->getMention(_currentMentionSet);
	const Mention* mentionToBeChanged;

	Symbol word = SymbolConstants::nullSymbol;
	if (!left->getEntityType().isRecognized()) {
		mentionToBeChanged = left;
	} else if (!right->getEntityType().isRecognized()) {
		mentionToBeChanged = right;
	} else return false;

	EntityType newType = EntityType::getOtherType();
	if (RelationUtilities::get()->coercibleToType(mentionToBeChanged,
		EntityType::getPERType().getName()))
	{
		newType = EntityType::getPERType();
	} else if (RelationUtilities::get()->coercibleToType(mentionToBeChanged,
		EntityType::getORGType().getName()))
	{
		newType = EntityType::getORGType();
	} else if (RelationUtilities::get()->coercibleToType(mentionToBeChanged,
		EntityType::getLOCType().getName()))
	{
		newType = EntityType::getLOCType();
	}

	if (newType.isRecognized()) {

		_instance->setStandardInstance(leftArg, rightArg, prop, _currentMentionSet);
		if (!left->getEntityType().isRecognized())
			_instance->setLeftEntityType(newType.getName());
		else _instance->setRightEntityType(newType.getName());

		return tryToCoerceInstance(left, right, mentionToBeChanged, newType);

	} else if (_relationFinderType == ITEA) {

		_instance->setStandardInstance(leftArg, rightArg, prop, _currentMentionSet);
		if (!left->getEntityType().isRecognized())
			_instance->setLeftEntityType(SpanishRelationModel::forcedOrgSym);
		else _instance->setRightEntityType(SpanishRelationModel::forcedOrgSym);

		if (tryToCoerceInstance(left, right, mentionToBeChanged, EntityType::getORGType()))
			return true;

		if (!left->getEntityType().isRecognized())
			_instance->setLeftEntityType(SpanishRelationModel::forcedPerSym);
		else _instance->setRightEntityType(SpanishRelationModel::forcedPerSym);
		return tryToCoerceInstance(left, right, mentionToBeChanged, EntityType::getPERType());
	}

	return false;
}

bool OldRelationFinder::tryToCoerceInstance(const Mention *left, const Mention *right,
										 const Mention *mentionToBeChanged, EntityType newType)
{
	if (_allow_type_coercing && classifyInstance(left, right)) {
		changeMentionType(mentionToBeChanged, newType);
		if (DEBUG) {
			_debugStream << L"***Changing mention type of: ";
			_debugStream << mentionToBeChanged->getNode()->toTextString() << L" to " ;
			_debugStream << newType.getName().to_string();
			_debugStream << L"***\n";
		}
		return true;
	}
	return false;
}

void OldRelationFinder::changeMentionType(const Mention *ment, EntityType type) {
	_currentMentionSet->changeEntityType(ment->getUID(), type);
}

bool OldRelationFinder::classifyInstance(const Mention *first, const Mention *second)
{
	if (first->getEntityType().isTemp() || second->getEntityType().isTemp())
		return false;

	if (DEBUG) {
		_debugStream << L"\nCONSIDERING: ";
		_debugStream << first->getNode()->toTextString();
		_debugStream << L" AND ";
		_debugStream << second->getNode()->toTextString();
		_debugStream << L"\n";

		if (_use_correct_answers) {
			// all this is unneccessary to actually run correct answer SERIF,
			// but it's not hurting anyone, and it's much easier to have it
			// in here under use_correct_answers than commented out.
			CorrectMention *firstCM = currentCorrectDocument->getCorrectMentionFromMentionID(first->getUID());
			CorrectMention *secondCM = currentCorrectDocument->getCorrectMentionFromMentionID(second->getUID());

			if (firstCM != NULL && secondCM != NULL) {
				_debugStream << firstCM->getAnnotationID().to_string() << L" " << secondCM->getAnnotationID().to_string() << L"\n";
				_debugStream << L"FIRST CM TOKENS: " << first->getNode()->getStartToken() << L" ";
				_debugStream << first->getNode()->getEndToken() << L"\n";
				_debugStream << L"SECOND CM TOKENS: " << second->getNode()->getStartToken() << L" ";
				_debugStream << second->getNode()->getEndToken() << L"\n";
			} else {
				_debugStream << "NO CORRECT MENTIONS\n";
			}
		}

		_debugStream << L"INSTANCE: " << _instance->toString() << L"\n";
	}

	// see if a special case fires, if so, use that...
	int relation_type
		= _specialRelationCases->findSpecialRelationType(_instance, first, second);


	// this is for situations like "he was killed in a city in Russia", where
	//   we have the vector: [killed GPE GPE in in] rather than
	//                       [city GPE GPE <ref> in]
	// artificiallyStackPrepositions() creates the latter from the former when called
	if (_specialRelationCases->isSpecialPrepositionStack(_instance, first, second)) {
		if (DEBUG) _debugStream << L"--artifically stacking--\n";
		RelationUtilities::get()->artificiallyStackPrepositions(_instance);
	}

	// ...if not, call on the model
	if (RelationTypeSet::isNull(relation_type))
		relation_type = _model->findBestRelationType(_instance);

	if (RelationTypeSet::isNull(relation_type))
		return false;

	if (RelationTypeSet::isReversed(relation_type)) {
		addRelation(second,	first, RelationTypeSet::reverse(relation_type), 0);
	} else {
		addRelation(first, second, relation_type, 0);
	}

	return true;
}


void OldRelationFinder::addRelation(const Mention *first,
								 const Mention *second,
								 int type,
								 float score)
{
	if (RelationTypeSet::isNull(type))
		return;

	if (RelationTypeSet::isReversed(type)) {
		const Mention *temp = first;
		first = second;
		second = temp;
		type = RelationTypeSet::reverse(type);
	}

	// distribute over sets
	if (first->getMentionType() == Mention::LIST) {
		const Mention *iter = first->getChild();
		while (iter != 0) {
			if (iter->getEntityType().isRecognized())
				addRelation(iter, second, type, score);
			iter = iter->getNext();
		}
		return;
	}
	if (second->getMentionType() == Mention::LIST) {


		// for ACE:
		// eliminate such examples as "sections of X and Y",
		// but keep things like "the owner of X and Y"
		// ??? just a guess ???
		if (_relationFinderType == ACE &&
			first->getNode()->getHeadPreterm()->getTag() == SpanishSTags::NNS &&
			(second->getNode()->getParent()->getParent() == first->getNode()))
		{
			if (DEBUG) {
				_debugStream << L"not distributing: ";
				_debugStream << first->getNode()->toTextString();
				_debugStream << L" & ";
				_debugStream << second->getNode()->toTextString();
				_debugStream << L"\n";
			}
			return;
		}

		// hierarchy relations for ITEA
		if (_relationFinderType == ITEA) //&&
			//type == RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getSubordinateType()))
		{
			int subordinate_type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getSubordinateType());

			Mention *stackedOrgs[10];
		    int n_orgs = RelationUtilities::get()->getOrgStack(second, stackedOrgs, 10);
			if (n_orgs != 0) {
				addRelation(first, stackedOrgs[0], type, 1.0);
				for (int i = 0; i < n_orgs - 1; i++) {
					addRelation(stackedOrgs[i], stackedOrgs[i+1], subordinate_type, 1.0);
				}
				return;
			}
		}

		const Mention *iter = second->getChild();
		while (iter != 0) {
			if (iter->getEntityType().isRecognized())
				addRelation(first, iter, type, score);
			iter = iter->getNext();
		}

		return;
	}

	Symbol symtype = RelationTypeSet::getNormalizedRelationSymbol(type);

	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			symtype, _currentSentenceIndex, _n_relations, score);
		if (DEBUG) _debugStream << _relations[_n_relations]->toString() << L"\n";
		_n_relations++;
	}

}

void OldRelationFinder::collectDefinitions(const PropositionSet *propSet) {

	_definitions.clear();
	for (int j = 0; j < propSet->getNPropositions(); j++) {
		_propTakesPlaceAt[j] = 0;
	}

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);
		if (prop->getPredType() == Proposition::NOUN_PRED ||
			prop->getPredType() == Proposition::MODIFIER_PRED) {
			int index = prop->getArg(0)->getMentionIndex();
			_definitions[index] = prop;

			if (prop->getArg(0)->getType() == Argument::MENTION_ARG &&
				prop->getArg(0)->getMention(_currentMentionSet)->getEntityType().isRecognized())
			{
				for (int j = 1; j < prop->getNArgs(); j++) {
					Argument *arg = prop->getArg(j);
					if (arg->getType() == Argument::PROPOSITION_ARG &&
						arg->getRoleSym() == SpanishWordConstants::WHERE)
					{
						_propTakesPlaceAt[arg->getProposition()->getIndex()] =
							prop->getArg(0);
					}
				}
			}
		}
	}
}


void OldRelationFinder::addRawRelations(Proposition *prop,
									 const MentionSet *mentionSet)
{
	RelMention *rawRelation = RawRelationFinder::getRawRelMention(
		prop, mentionSet, _currentSentenceIndex, _n_relations);

	if (rawRelation &&
		_n_relations < MAX_SENTENCE_RELATIONS)
	{
		_relations[_n_relations] = rawRelation;
		_n_relations++;
	}
}


void OldRelationFinder::addPartitiveRelations() {
	for (int i = 0; i < _currentMentionSet->getNMentions(); i++) {
		Mention *ment = _currentMentionSet->getMention(i);
		if (ment->getMentionType() != Mention::PART ||
			ment->getChild() == 0)
			continue;
		Symbol word = ment->getNode()->getHeadWord();
		Symbol inner_word = ment->getChild()->getNode()->getHeadWord();
		if (ment->getEntityType().isRecognized()) {
			if (WordNet::getInstance()->isNumberWord(word))
				word = number_symbol;
			_instance->setPartitiveInstance(word, inner_word, ment->getEntityType().getName());
			classifyInstance(ment, ment->getChild());
		} else if (_relationFinderType == EELD) {
			// "[] of them"
			if (_allow_type_coercing && inner_word == SpanishWordConstants::THEM) {
				changeMentionType(ment, EntityType::getPERType());
				changeMentionType(ment->getChild(), EntityType::getPERType());

				if (DEBUG) {
					_debugStream << L"***Changing mention type of partitive: ";
					_debugStream << ment->getNode()->toTextString() << L" to " ;
					_debugStream << EntityType::getPERType().getName().to_string();
					_debugStream << L"***\n";
				}
				if (WordNet::getInstance()->isNumberWord(word))
					word = number_symbol;
				_instance->setPartitiveInstance(word, inner_word, ment->getEntityType().getName());
				classifyInstance(ment, ment->getChild());
			}
		}
	}
}

