// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_PotentialRelationCollector.h"
#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Generic/preprocessors/RelationTypeMap.h"
#include "Chinese/relations/ch_PotentialRelationInstance.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8InputStream.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

const Symbol NONE_SYM = Symbol(L"NONE");
const Symbol NULL_SYM = Symbol();
const Symbol BAD_EDT_SYM = Symbol(L"BAD EDT");
const Symbol SAME_ENTITY_SYM = Symbol(L"SAME ENTITY");
const Symbol NEG_HYPO_SYM = Symbol(L"NEG/HYPO");

ChinesePotentialRelationCollector::ChinesePotentialRelationCollector(int collectionMode, RelationTypeMap *relationTypes) :
												_docTheory(0),  _mentionSet(0)
{
	_collectionMode = collectionMode;
	_relationTypes = relationTypes;

	if (_collectionMode != EXTRACT &&
		_collectionMode != TRAIN &&
		_collectionMode != CLASSIFY &&
		_collectionMode != CORRECT_ANSWERS)
	{
		char c[200];
		sprintf(c, "%d is not a valid relation collection mode: use EXTRACT, TRAIN, CLASSIFY or CORRECT_ANSWERS", _collectionMode);
				throw InternalInconsistencyException("ChinesePotentialRelationCollector::ChinesePotentialRelationCollector()", c);
	}

	if (_collectionMode == CORRECT_ANSWERS) {
		std::string packet = ParamReader::getRequiredParam("relation_packet_file");
		std::string ca_file = ParamReader::getRequiredParam("correct_answer_file");
		_annotationSet = readPacketAnnotation(packet.c_str());
		_outputStream.open(ca_file.c_str());
		_outputStream << "(\n";
	}

	_n_relations = 0;
}

ChinesePotentialRelationCollector::~ChinesePotentialRelationCollector() {
	if (_collectionMode == CORRECT_ANSWERS) {
		_outputStream << ")\n";
		_outputStream.close();
	}
	finalize();
}

void ChinesePotentialRelationCollector::resetForNewSentence() {
	if (_collectionMode != TRAIN && _collectionMode != CORRECT_ANSWERS)
		finalize(); // get rid of old stuff
}

void ChinesePotentialRelationCollector::loadDocTheory(DocTheory* theory) {
	if (_collectionMode != TRAIN)
		finalize(); // get rid of old stuff
	_docTheory = theory;
}

void ChinesePotentialRelationCollector::produceOutput(const wchar_t *output_dir,
											   const wchar_t *doc_filename)
{
	collectPotentialDocumentRelations();

	if (_collectionMode == EXTRACT && _trainingRelations.length() > 0) {
		wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".relsent";

		_outputStream.open(output_file.c_str());
		outputPotentialTrainingRelations();
		_outputStream.close();
		_outputFiles.add(output_file);
	}

	else if (_collectionMode == CORRECT_ANSWERS) {
		outputCorrectAnswerFile();
	}

}

void ChinesePotentialRelationCollector::collectPotentialDocumentRelations() {
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		Parse *parse = _docTheory->getSentenceTheory(i)->getPrimaryParse();
		PropositionSet *propSet = _docTheory->getSentenceTheory(i)->getPropositionSet();
		MentionSet *mentionSet = _docTheory->getSentenceTheory(i)->getMentionSet();
		collectPotentialSentenceRelations(parse, mentionSet, propSet);
	}
}

void ChinesePotentialRelationCollector::collectPotentialSentenceRelations(const Parse *parse,
																   MentionSet *mentionSet,
																   const PropositionSet *propSet)
{
	_mentionSet = mentionSet;

	std::vector<bool> is_false_or_hypothetical = RelationUtilities::get()->identifyFalseOrHypotheticalProps(propSet, _mentionSet);
	collectDefinitions(propSet);

	// Debug output
	if (RelationUtilities::get()->debugStreamIsOn()) {
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			propSet->getProposition(i)->dump(RelationUtilities::get()->getDebugStream(), 0);
			if (is_false_or_hypothetical[propSet->getProposition(i)->getIndex()])
				RelationUtilities::get()->getDebugStream() << L" -- UNTRUTH";
			RelationUtilities::get()->getDebugStream() << L"\n";
		}
		RelationUtilities::get()->getDebugStream() << parse->getRoot()->toTextString() << L"\n";
		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *ment = mentionSet->getMention(j);
			if (ment->getEntityType().isRecognized())
				RelationUtilities::get()->getDebugStream() << ment->getNode()->toTextString() << ": " << ment->getEntityType().getName().to_string() << "\n";
		}
		RelationUtilities::get()->getDebugStream() << parse->getRoot()->toPrettyParse(0) << L"\n";
	}

	for (int m = 0; m < propSet->getNPropositions(); m++) {
		Proposition *prop = propSet->getProposition(m);

		if (is_false_or_hypothetical[prop->getIndex()])
			continue;

		if (prop->getPredType() == Proposition::COPULA_PRED) {
			classifyCopulaProposition(prop);
		} else if (prop->getPredType() == Proposition::LOC_PRED) {
			; //classifyLocationProposition(prop); - special case: no classification
		// JCS - 03/12/04 - Don't want to classify members of set with one another
		// JCS - 04/02/04 - Okay, maybe we do - there are some examples in the 2003 ACE Eval data
		} else if (prop->getPredType() == Proposition::SET_PRED) {
			classifySetProposition(prop);
		} else if (prop->getPredType() == Proposition::COMP_PRED) {
			classifyCompProposition(prop);
		} else if (prop->getPredType() == Proposition::VERB_PRED ||
				prop->getPredType() == Proposition::POSS_PRED ||
				prop->getPredType() == Proposition::MODIFIER_PRED ||
				prop->getPredType() == Proposition::NOUN_PRED) {
			classifyProposition(prop);
		}
	}
}

void ChinesePotentialRelationCollector::classifyCopulaProposition(Proposition *prop) {

	const Mention *subject = 0;
	const Mention *object = 0;

	for (int i = 0; i < prop->getNArgs(); i++) {
		Argument *arg = prop->getArg(i);
		if (arg->getType() == Argument::MENTION_ARG) {
			if (arg->getRoleSym() == Argument::SUB_ROLE) {
				subject = arg->getMention(_mentionSet);
			} else if (arg->getRoleSym() == Argument::OBJ_ROLE) {
				object = arg->getMention(_mentionSet);
			}
		}
	}

	if (subject == 0 || object == 0)
		return;

	// Special Cases - not implemented yet in Chinese
	/*int type = RelationTypeSet::getTypeFromSymbol(SpecialRelationCases::getGroupMembers());
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
	}*/

	classifyProposition(prop);
}

void ChinesePotentialRelationCollector::classifySetProposition(Proposition *prop) {

	Argument *whereArg = 0;

	// start at 1 instead of 0 to skip the LIST mention
	for (int i = 1; i < prop->getNArgs(); i++) {
		if (prop->getArg(i)->getType() != Argument::MENTION_ARG)
			continue;

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

void ChinesePotentialRelationCollector::classifyCompProposition(Proposition *prop) {
	// If there are prepositional phrases or other modifiers
	// attached to the comp, we want to distribute those to
	// each member proposition.
	for (int i = 0; i < prop->getNArgs(); i++) {
		if (prop->getArg(i)->getType() == Argument::MENTION_ARG) {
			for (int j = i + 1; j < prop->getNArgs(); j++) {
				if (prop->getArg(j)->getType() == Argument::PROPOSITION_ARG)
					classifyPropArgumentPair(prop->getArg(i), prop->getArg(j), prop, false);
			}
		}
		if (prop->getArg(i)->getType() == Argument::PROPOSITION_ARG) {
			for (int j = i + 1; j < prop->getNArgs(); j++) {
				if (prop->getArg(j)->getType() == Argument::MENTION_ARG)
					classifyPropArgumentPair(prop->getArg(i), prop->getArg(j), prop, true);
			}
		}
	}
}


void ChinesePotentialRelationCollector::classifyProposition(Proposition *prop) {

	bool prop_is_definition_of_EDT_entity = false;
	if (prop->getPredType() == Proposition::NOUN_PRED &&
		prop->getArg(0)->getType() == Argument::MENTION_ARG &&
		prop->getArg(0)->getMention(_mentionSet)->getEntityType().isRecognized())
	{	prop_is_definition_of_EDT_entity = true; }

	Argument *whereArg = 0;
	/*if (_propTakesPlaceAt[prop->getIndex()] != 0) {
		whereArg = _new Argument();
		whereArg->populateWithMention(in_symbol,
			_propTakesPlaceAt[prop->getIndex()]->getMentionIndex());
	}*/

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
		if (prop->getArg(i)->getType() != Argument::MENTION_ARG &&
			prop->getArg(i)->getType() != Argument::PROPOSITION_ARG)
			continue;

		// we don't nest through EDT entities (e.g. _his_ house in _Limassol_)
		if (prop_is_definition_of_EDT_entity &&	i > 0) {
			delete whereArg;
			return;
		}

		if (prop->getArg(i)->getType() == Argument::MENTION_ARG) {

			for (int j = i + 1; j < prop->getNArgs(); j++) {
				if (prop->getArg(j)->getType() == Argument::MENTION_ARG)
					classifyArgumentPair(prop->getArg(i), prop->getArg(j), prop);
				if (prop->getArg(j)->getType() == Argument::PROPOSITION_ARG)
					classifyPropArgumentPair(prop->getArg(i), prop->getArg(j), prop, false);
			}

			if (whereArg != 0) {
				classifyArgumentPair(prop->getArg(i), whereArg, prop);
			}
		}

		if (prop->getArg(i)->getType() == Argument::PROPOSITION_ARG) {

			for (int j = i + 1; j < prop->getNArgs(); j++) {
				if (prop->getArg(j)->getType() == Argument::MENTION_ARG)
					classifyPropArgumentPair(prop->getArg(i), prop->getArg(j), prop, true);
			}
		}

	}

	delete whereArg;

}

void ChinesePotentialRelationCollector::classifyArgumentPair(Argument *leftArg, Argument *rightArg,
															Proposition *prop)
{
	const Mention* left = leftArg->getMention(_mentionSet);
	const Mention* right = rightArg->getMention(_mentionSet);

	// if leftArg and rightArg are both EDT entities, add to
	// _relations
	//
	// if one is and one isn't, but the non-EDT entity is a list,
	// go to FindListInstances
	//
	// if one is and one isn't, and the non-EDT entity isn't a list,
	// go to findNestedInstances
	//
	// if neither are, return.

	if (left->getEntityType().isRecognized() &&
		right->getEntityType().isRecognized())
	{
		addPotentialRelation(leftArg, rightArg, prop);
	} else if (left->getEntityType().isRecognized() &&
		       right->getMentionType() == Mention::LIST) {
		findListInstances(leftArg, rightArg, prop, false);
	} else if (right->getEntityType().isRecognized() &&
		       left->getMentionType() == Mention::LIST) {
		findListInstances(leftArg, rightArg, prop, true);
	} else if (left->getEntityType().isRecognized()) {
		//if (!findWithCoercedType(leftArg, rightArg, prop))  No coercion yet
			findNestedInstances(leftArg, rightArg, prop);
	} else if (right->getEntityType().isRecognized()) {
		// LB: we don't nest on this side (for now)
		; //findWithCoercedType(leftArg, rightArg, prop);    No coercion yet
	}
}

void ChinesePotentialRelationCollector::classifyPropArgumentPair(Argument *left, Argument *right,
														  Proposition *prop, bool leftIsProp)
{
	const Proposition *inner_term;
	if (leftIsProp) {
		inner_term = left->getProposition();
		if (!right->getMention(_mentionSet)->getEntityType().isRecognized())
			return;
	}
	else {
		inner_term = right->getProposition();
		if (!left->getMention(_mentionSet)->getEntityType().isRecognized())
			return;
	}

	for (int i = 0; i < inner_term->getNArgs(); i++) {
		if (inner_term->getArg(i)->getType() == Argument::PROPOSITION_ARG) {
			if (leftIsProp)
				classifyPropArgumentPair(inner_term->getArg(i), right, prop, true);
			else
				classifyPropArgumentPair(left, inner_term->getArg(i), prop, false);
		}
		if (inner_term->getArg(i)->getType() != Argument::MENTION_ARG)
			continue;
		if (!inner_term->getArg(i)->getMention(_mentionSet)->getEntityType().isRecognized())
			continue;
		if (leftIsProp)
			addNestedPotentialRelation(right, left, inner_term->getArg(i), prop, inner_term);
		else
			addNestedPotentialRelation(left, right, inner_term->getArg(i), prop, inner_term);
	}

}

/** this function deals specifically with lists of type OTH
 *  with children of only one EDT type
 *
 *  for example: in "a boxer and an emigre", we don't get "a boxer" as
 *  a PER, so the list is of type OTH. Same with "Bob, 52," (annoying but true)
 *
 */
void ChinesePotentialRelationCollector::findListInstances(Argument *left, Argument *right,
														 Proposition *prop, bool leftIsList)
{

	const Mention *iter;
	if (leftIsList)
		iter = left->getMention(_mentionSet)->getChild();
	else iter = right->getMention(_mentionSet)->getChild();

	while (iter != 0) {
		if (iter->getEntityType().isRecognized() && _definitions[iter->getIndex()] != 0) {
			if (leftIsList)
				addPotentialRelation(_definitions[iter->getIndex()]->getArg(0), right, prop);
			else addPotentialRelation(left, _definitions[iter->getIndex()]->getArg(0), prop);
		}
		iter = iter->getNext();
	}
}

void ChinesePotentialRelationCollector::findNestedInstances(Argument *first, Argument *intermediate,
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
		if (inner_term->getArg(i)->getType() == Argument::PROPOSITION_ARG)
			// example: "He found the cat that was in the barn" ??
			classifyPropArgumentPair(first, inner_term->getArg(i), prop, false);
		if (inner_term->getArg(i)->getType() != Argument::MENTION_ARG)
			continue;
		if (!inner_term->getArg(i)->getMention(_mentionSet)->getEntityType().isRecognized())
			continue;
		addNestedPotentialRelation(first, intermediate, inner_term->getArg(i), prop, inner_term);
	}
}




void ChinesePotentialRelationCollector::addPotentialRelation(Argument *arg1, Argument *arg2, const Proposition *prop) {

	// return if same mention
	if (arg1->getMentionIndex() == arg2->getMentionIndex())
		return;

	// return if mentions of the same entity (we only have access to entity set in EXTRACT or TRAIN mode)
	if (_collectionMode == EXTRACT || _collectionMode == TRAIN) {
		if (_docTheory->getEntitySet()->getEntityByMention(
			arg1->getMention(_mentionSet)->getUID(), arg1->getMention(_mentionSet)->getEntityType()) ==
		_docTheory->getEntitySet()->getEntityByMention(
			arg2->getMention(_mentionSet)->getUID(), arg2->getMention(_mentionSet)->getEntityType()))
		return;
	}

	Proposition *def = _definitions[arg1->getMentionIndex()];
	if (def != NULL && def->getPredType() == Proposition::SET_PRED) {
		distributeOverSet(def, arg2, true, prop);
		return;
	}
	def = _definitions[arg2->getMentionIndex()];
	if (def != NULL && def->getPredType() == Proposition::SET_PRED) {
		distributeOverSet(def, arg1, false, prop);
		return;
	}

	if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
		PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg1, arg2, _docTheory,
																		 _mentionSet->getSentenceNumber());
		if (_relationSet.find(*tRel) != _relationSet.end()) {
			delete tRel;
			return;
		}
		_relationSet[*tRel] = true;
		_trainingRelations.add(tRel);
	}
	if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
		PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
		iRel->setStandardInstance(arg1, arg2, prop, _mentionSet);
		_relationInstances.add(iRel);
	}

	//addMetonymicRelations(arg1, arg2, prop);

	_n_relations++;
}

void ChinesePotentialRelationCollector::distributeOverSet(Proposition *set, Argument *arg,
													  bool set_is_arg1, const Proposition *prop)
{

	for (int j = 1; j < set->getNArgs(); j++) {
		Argument *memberArg = set->getArg(j);
		if (memberArg->getType() == Argument::MENTION_ARG &&
			memberArg->getMention(_mentionSet)->isOfRecognizedType())
		{
			if (set_is_arg1)
				addPotentialRelation(memberArg, arg, prop);
			else
				addPotentialRelation(arg, memberArg, prop);
		}
	}

}

void ChinesePotentialRelationCollector::distributeOverNestedSet(Proposition *set, Argument *arg,
															   Argument *intermediate,
															   bool set_is_arg1,
															   const Proposition *outer_arg,
															   const Proposition *inner_arg)
{

	for (int j = 1; j < set->getNArgs(); j++) {
		Argument *memberArg = set->getArg(j);
		if (memberArg->getType() == Argument::MENTION_ARG &&
			memberArg->getMention(_mentionSet)->isOfRecognizedType())
		{
			if (set_is_arg1)
				addNestedPotentialRelation(memberArg, intermediate, arg, outer_arg, inner_arg);
			else
				addNestedPotentialRelation(arg, intermediate, memberArg, outer_arg, inner_arg);
		}
	}

}

void ChinesePotentialRelationCollector::addNestedPotentialRelation(Argument *arg, Argument *intermediate,
																  Argument *nested, const Proposition *outer_arg,
																  const Proposition *inner_arg)
{
	if (arg->getMentionIndex() == nested->getMentionIndex())
		return;

	// return if mentions of the same entity (we only have access to entity set in EXTRACT or TRAIN mode)
	if (_collectionMode == EXTRACT || _collectionMode == TRAIN) {
		if (_docTheory->getEntitySet()->getEntityByMention(
			arg->getMention(_mentionSet)->getUID(), arg->getMention(_mentionSet)->getEntityType()) ==
			_docTheory->getEntitySet()->getEntityByMention(
			nested->getMention(_mentionSet)->getUID(), nested->getMention(_mentionSet)->getEntityType()))
			return;
	}

	Proposition *def = _definitions[arg->getMentionIndex()];
	if (def != NULL && def->getPredType() == Proposition::SET_PRED) {
		distributeOverNestedSet(def, nested, intermediate, true, outer_arg, inner_arg);
		return;
	}
	def = _definitions[nested->getMentionIndex()];
	if (def != NULL && def->getPredType() == Proposition::SET_PRED) {
		distributeOverNestedSet(def, arg, intermediate, false, outer_arg, inner_arg);
		return;
	}

	if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
		PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg, nested, _docTheory,
																_mentionSet->getSentenceNumber());
		if (_relationSet.find(*tRel) != _relationSet.end()) {
			delete tRel;
			return;
		}
		_relationSet[*tRel] = true;
		_trainingRelations.add(tRel);
	}
	if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
		PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
		iRel->setStandardNestedInstance(arg, intermediate, nested, outer_arg, inner_arg, _mentionSet);
		_relationInstances.add(iRel);
	}

	//addMetonymicNestedRelations(arg, intermediate, nested, outer_arg, inner_arg);

	_n_relations++;
}



void ChinesePotentialRelationCollector::outputPotentialTrainingRelations() {
	_outputStream << _trainingRelations.length() << L"\n";
	for (int i = 0; i < _trainingRelations.length(); i++) {
		_outputStream << *_trainingRelations[i];
	}
}

void ChinesePotentialRelationCollector::outputCorrectAnswerFile() {
	_outputStream << "(" << _docTheory->getDocument()->getName().to_string() << "\n";
	outputCorrectEntities();
	outputCorrectRelations();
	_outputStream << ")" << "\n";
}

void ChinesePotentialRelationCollector::outputCorrectEntities() {
	_outputStream << "  (Entities" << "\n";

	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		MentionSet *mentions = _docTheory->getSentenceTheory(i)->getMentionSet();
		TokenSequence *tokens = _docTheory->getSentenceTheory(i)->getTokenSequence();
		for (int j = 0; j < mentions->getNMentions(); j++) {
			Mention *m = mentions->getMention(j);
			if (m->isOfRecognizedType() &&
			    (m->getMentionType() == Mention::NAME ||
				 m->getMentionType() == Mention::DESC ||
				 m->getMentionType() == Mention::PART ||
				 m->getMentionType() == Mention::PRON))
			{
				_outputStream << "    (" << m->getEntityType().getName().to_string();
				_outputStream << " " << getDefaultSubtype(m->getEntityType()).to_string();
				_outputStream << " SPC FALSE E" << m->getUID() << "\n";
				EDTOffset start = tokens->getToken(m->getNode()->getStartToken())->getStartEDTOffset();
				EDTOffset end = tokens->getToken(m->getNode()->getEndToken())->getEndEDTOffset();
				_outputStream << "      (" << convertMentionType(m) << " ";
				if (m->hasRoleType())
					_outputStream << m->getRoleType().getName().to_string() << " ";
				else
					_outputStream << "NONE ";
				_outputStream << start << " " << end;
				_outputStream << " " << start << " " << end << " " << m->getUID() << "-1)\n";
				_outputStream << "    )\n";
			}
		}
	}
	_outputStream << "  )" << "\n";
}


void ChinesePotentialRelationCollector::outputCorrectRelations() {
	_outputStream << "  (Relations" << "\n";

	for (int i = 0; i < getNRelations(); i++) {
		PotentialTrainingRelation *rel = getPotentialTrainingRelation(i);
		PotentialRelationMap::iterator it = _annotationSet->find(*rel);
		if (it != _annotationSet->end())  {
			PotentialTrainingRelation ann = ((*it).first);
			if (ann.getRelationType() != BAD_EDT_SYM &&
				ann.getRelationType() != SAME_ENTITY_SYM &&
				ann.getRelationType() != NEG_HYPO_SYM &&
				ann.getRelationType() != NONE_SYM)
			{
				Symbol type = _relationTypes->lookup(ann.getRelationType());
				_outputStream << "    (" << type.to_string() << " EXPLICIT R" << i << "\n";
				if (!ann.relationIsReversed()) {
					_outputStream << "      (" << rel->getLeftMention() << "-1 ";
					_outputStream << rel->getRightMention() << "-1 " << i << "-1)\n";
				}
				else {
					_outputStream << "      (" << rel->getRightMention() << "-1 ";
					_outputStream << rel->getLeftMention() << "-1 " << i << "-1)\n";
				}
				_outputStream << "    )\n";
			}
			else if (ann.getRelationType() == NEG_HYPO_SYM ||
					 ann.getRelationType() == NONE_SYM)
			{
				_outputStream << "    (" << NONE_SYM.to_string() << " EXPLICIT R" << i << "\n";
				if (!ann.relationIsReversed()) {
					_outputStream << "      (" << rel->getLeftMention() << "-1 ";
					_outputStream << rel->getRightMention() << "-1 " << i << "-1)\n";
				}
				else {
					_outputStream << "      (" << rel->getRightMention() << "-1 ";
					_outputStream << rel->getLeftMention() << "-1 " << i << "-1)\n";
				}
				_outputStream << "    )\n";
			}

		}

	}
	_outputStream << "  )" << "\n";
}


void ChinesePotentialRelationCollector::finalize() {
	while (_trainingRelations.length() > 0) {
		PotentialTrainingRelation *r = _trainingRelations.removeLast();
		delete r;
	}
	while (_relationInstances.length() > 0) {
		PotentialRelationInstance *r = _relationInstances.removeLast();
		delete r;
	}

	_relationSet.clear();
	_n_relations = 0;
}

void ChinesePotentialRelationCollector::outputPacketFile(const char *output_dir,
												  const char *packet_name)
{
	std::stringstream packet_file;
	packet_file << output_dir << SERIF_PATH_SEP << packet_name << ".pkt";

	ofstream packetStream;
	packetStream.open(packet_file.str().c_str());
	packetStream << _outputFiles.length() << "\n";
	for (int i = 0; i < _outputFiles.length(); i++) {
		packetStream << _outputFiles[i] << "\n";
	}
	packetStream.close();

}

PotentialTrainingRelation* ChinesePotentialRelationCollector::getPotentialTrainingRelation(int i) {
	if (i < 0 || i > _trainingRelations.length() - 1)
		throw InternalInconsistencyException("ChinesePotentialRelationCollector::getPotentialTrainingRelation",
											 "Array index out of bounds");
	else
		return _trainingRelations[i];
}

PotentialRelationInstance* ChinesePotentialRelationCollector::getPotentialRelationInstance(int i) {
	if (i < 0 || i > _relationInstances.length() - 1)
		throw InternalInconsistencyException("ChinesePotentialRelationCollector::getPotentialRelationInstance",
											 "Array index out of bounds");
	else
		return _relationInstances[i];
}

void ChinesePotentialRelationCollector::collectDefinitions(const PropositionSet *propSet) {

	_definitions.clear();
	/*for (int j = 0; j < propSet->getNPropositions(); j++) {
		_propTakesPlaceAt[j] = 0;
	}*/

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);
		if (prop->getPredType() == Proposition::NOUN_PRED ||
			prop->getPredType() == Proposition::NAME_PRED ||
			prop->getPredType() == Proposition::MODIFIER_PRED ||
			prop->getPredType() == Proposition::SET_PRED ||
			prop->getPredType() == Proposition::PRONOUN_PRED) {
			int index = prop->getArg(0)->getMentionIndex();
			_definitions[index] = prop;
			/*if (prop->getArg(0)->getType() == Argument::MENTION_ARG &&
				prop->getArg(0)->getMention(_mentionSet)->getEntityType().isRecognized())
			{
				for (int j = 1; j < prop->getNArgs(); j++) {
					Argument *arg = prop->getArg(j);
					if (arg->getType() == Argument::PROPOSITION_ARG &&
						arg->getRoleSym() == ChineseWordConstants::WHERE)
					{
						_propTakesPlaceAt[arg->getProposition()->getIndex()] =
							prop->getArg(0);
					}
				}
			}*/
		}
	}
}

void ChinesePotentialRelationCollector::addMetonymicRelations(Argument *arg1, Argument *arg2, const Proposition *prop) {

	if (arg1->getMention(_mentionSet)->hasIntendedType()) {
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg1, arg2,
																			 _docTheory,
																			 _mentionSet->getSentenceNumber(),
																			 true, false);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardInstance(arg1, arg2, prop, _mentionSet);
			iRel->setLeftEntityType(arg1->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
	if (arg2->getMention(_mentionSet)->hasIntendedType()) {
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg1, arg2,
																			_docTheory,
																			_mentionSet->getSentenceNumber(),
																			false, true);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardInstance(arg1, arg2, prop, _mentionSet);
			iRel->setRightEntityType(arg2->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
	if (arg1->getMention(_mentionSet)->hasIntendedType() && arg2->getMention(_mentionSet)->hasIntendedType()) {
		EntityType lIntended = arg1->getMention(_mentionSet)->getIntendedType();
		EntityType rIntended = arg2->getMention(_mentionSet)->getIntendedType();
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg1, arg2,
																			_docTheory,
																			_mentionSet->getSentenceNumber(),
																			true, true);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardInstance(arg1, arg2, prop, _mentionSet);
			iRel->setLeftEntityType(arg1->getMention(_mentionSet)->getIntendedType().getName());
			iRel->setRightEntityType(arg2->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
}

void ChinesePotentialRelationCollector::addMetonymicNestedRelations(Argument *arg, Argument *intermediate,
															 Argument *nested, const Proposition *outer_arg,
															 const Proposition *inner_arg) {
	if (arg->getMention(_mentionSet)->hasIntendedType()) {
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg, nested,
																			 _docTheory,
																			 _mentionSet->getSentenceNumber(),
																			 true, false);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardNestedInstance(arg, intermediate, nested, outer_arg, inner_arg, _mentionSet);
			iRel->setLeftEntityType(arg->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
	if (nested->getMention(_mentionSet)->hasIntendedType()) {
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg, nested,
																			 _docTheory,
																			 _mentionSet->getSentenceNumber(),
																			 false, true);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardNestedInstance(arg, intermediate, nested, outer_arg, inner_arg, _mentionSet);
			iRel->setRightEntityType(arg->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
	if (arg->getMention(_mentionSet)->hasIntendedType() && nested->getMention(_mentionSet)->hasIntendedType()) {
		if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
			PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(arg, nested,
																			 _docTheory,
																			 _mentionSet->getSentenceNumber(),
																			 true, true);
			_relationSet[*tRel] = true;
			_trainingRelations.add(tRel);
		}
		if (_collectionMode == TRAIN || _collectionMode == CLASSIFY) {
			PotentialRelationInstance *iRel = _new ChinesePotentialRelationInstance();
			iRel->setStandardNestedInstance(arg, intermediate, nested, outer_arg, inner_arg, _mentionSet);
			iRel->setLeftEntityType(arg->getMention(_mentionSet)->getIntendedType().getName());
			iRel->setRightEntityType(arg->getMention(_mentionSet)->getIntendedType().getName());
			_relationInstances.add(iRel);
		}
		_n_relations++;
	}
}

const wchar_t* ChinesePotentialRelationCollector::convertMentionType(Mention* ment)
{
	Mention::Type type = ment->mentionType;
	Mention* subMent = 0;
	switch (type) {
		case Mention::NAME:
			return L"NAME";
		case Mention::DESC:
		case Mention::PART:
			return L"NOMINAL";
		case Mention::PRON:
			return L"PRONOUN";
		case Mention::APPO:
			// in ACE 2004, we shouldn't be printing these
			return L"WARNING_BAD_MENTION_TYPE";
		case Mention::LIST:
			// for list, the type is the type of the first child
			subMent = ment->getChild();
			return convertMentionType(subMent);
		default:
			return L"WARNING_BAD_MENTION_TYPE";
	}
}

Symbol ChinesePotentialRelationCollector::getDefaultSubtype(EntityType type) {
	if (type.matchesPER())
		return Symbol(L"NONE");
	else if (type.matchesLOC())
		return Symbol(L"Address");
	else
		return Symbol(L"Other");
}

ChinesePotentialRelationCollector::PotentialRelationMap *ChinesePotentialRelationCollector::readPacketAnnotation
																			(const char *packet_file)
{
	PotentialRelationMap *annotationSet = _new PotentialRelationMap(1024);
	boost::scoped_ptr<UTF8InputStream> packet_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& packet(*packet_scoped_ptr);
	wchar_t line[200];

	packet.open(packet_file);
	if (packet.fail()) {

		char message[300];
		sprintf(message, "Could not open packet file `%s'", packet_file);
		throw UnexpectedInputException("ChinesePotentialRelationCollector::readPacketAnnotation()", message);
	}
	packet.getLine(line, 200);
	int n_packet_files = _wtoi(line);
	for (int i = 0; i < n_packet_files; i++) {
		PotentialTrainingRelation rel;
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);

		if (packet.eof()) {
			char message[300];
			sprintf(message, "Reached unexpected end of `%s'", packet_file);
			throw UnexpectedInputException("ChinesePotentialRelationCollector::readPacketAnnotation()",  message);
		}
		packet.getLine(line, 200);
		in.open(line);
		if (in.fail()) {
			char message[300];
			sprintf(message, "Could not open annotation file `%s'", UnicodeUtil::toUTF8String(line));
			throw UnexpectedInputException("ChinesePotentialRelationCollector::readPacketAnnotation()", message);
		}
		in.getLine(line, 200);
		int n_relations = _wtoi(line);
		for (int j = 0; j < n_relations; j++) {
			in >> rel;
			(*annotationSet)[rel] = rel.getRelationType();
		}
		in.close();

	}
	packet.close();
	return annotationSet;
}
