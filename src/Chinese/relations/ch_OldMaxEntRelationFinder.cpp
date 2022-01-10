// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_OldMaxEntRelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Chinese/relations/ch_RelationModel.h"
#include "Chinese/relations/ch_PotentialRelationCollector.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/common/WordConstants.h"

#include "Generic/CASerif/correctanswers/CorrectMention.h"

UTF8OutputStream OldMaxEntRelationFinder::_debugStream;
bool OldMaxEntRelationFinder::DEBUG = false;

static Symbol number_symbol(L"<number>");

OldMaxEntRelationFinder::OldMaxEntRelationFinder() {
	RelationTypeSet::initialize();

	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

	DEBUG = false;
	std::string debug_buffer = ParamReader::getParam("relation_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}

	std::string model_prefix = ParamReader::getParam("relation_model");
	_model = RelationModel::build(model_prefix.c_str());
	_collector = PotentialRelationCollector::build(ChinesePotentialRelationCollector::CLASSIFY);
}

OldMaxEntRelationFinder::~OldMaxEntRelationFinder() {
	delete _model;
	delete _collector;
}


void OldMaxEntRelationFinder::resetForNewSentence() {

	if (DEBUG) _debugStream << L"*** NEW SENTENCE ***\n";
	_n_relations = 0;
	_collector->resetForNewSentence();

}

RelMentionSet *OldMaxEntRelationFinder::getRelationTheory(const Parse *parse,
											   MentionSet *mentionSet,
											   PropositionSet *propSet,
											   const Parse *secondaryParse)
{
	_currentMentionSet = mentionSet;
	_currentSentenceIndex = mentionSet->getSentenceNumber();

	if (DEBUG && _currentSentenceIndex == 0) _debugStream << L"*** NEW DOCUMENT ***\n";

	_collector->collectPotentialSentenceRelations(parse, mentionSet, propSet);

	for (int i = 0; i < _collector->getNRelations(); i++) {

		PotentialRelationInstance *instance = _collector->getPotentialRelationInstance(i);
		const Mention *first = mentionSet->getMention(instance->getLeftMention());
		const Mention *second = mentionSet->getMention(instance->getRightMention());

		if (!first->getEntityType().canBeRelArg() || !second->getEntityType().canBeRelArg())
			continue;

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
				} else {
					_debugStream << "NO CORRECT MENTIONS\n";
				}
			}
			_debugStream << L"INSTANCE: " << instance->toString() << L"\n";
		}

		int relation_type = _model->findBestRelationType(instance);

		if (RelationTypeSet::isNull(relation_type))
			continue;

		if (RelationTypeSet::isReversed(relation_type)) {
			addRelation(second,	first, RelationTypeSet::reverse(relation_type), 0);
		} else {
			addRelation(first, second, relation_type, 0);
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




void OldMaxEntRelationFinder::addRelation(const Mention *first,
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

		const Mention *iter = second->getChild();
		while (iter != 0) {
			if (iter->getEntityType().isRecognized())
				addRelation(first, iter, type, score);
			iter = iter->getNext();
		}
		return;
	}

	// In case an appositive happends to slip thorough, use its name mention instead
	if (!PropositionFinder::getUnifyAppositives()) {
		while (first->getMentionType() == Mention::APPO) {
			first = first->getChild();
			first = first->getNext();
		}
		while (second->getMentionType() == Mention::APPO) {
			second = second->getChild();
			second = second->getNext();
		}
	}

	Symbol symtype = RelationTypeSet::getNormalizedRelationSymbol(type);

	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			symtype, _currentSentenceIndex, _n_relations, score);
		if (DEBUG) _debugStream << _relations[_n_relations]->toString() << L"\n";
		_n_relations++;
	}

}
