// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_MaxEntRelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Chinese/relations/discmodel/ch_P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/VectorModel.h"

UTF8OutputStream ChineseMaxEntRelationFinder::_debugStream;
bool ChineseMaxEntRelationFinder::DEBUG = false;

ChineseMaxEntRelationFinder::ChineseMaxEntRelationFinder() {

	ChineseP1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	std::string debug_buffer = ParamReader::getParam("relation_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}


	std::string tag_set_file = ParamReader::getRequiredParam("relation_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	
	std::string features_file = ParamReader::getRequiredParam("relation_features_file");
	DTFeatureTypeSet *featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);

	std::string model_file = ParamReader::getParam("maxent_relation_model_file");

	DTFeature::FeatureWeightMap *weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*weights, model_file.c_str(), P1RelationFeatureType::modeltype);

	_decoder = _new MaxEntModel(_tagSet, featureTypes, weights);

	//mrf - The relation validation string is used to determine which 
	//relation-type/argument types the decoder allows.  RelationObservation calls 
	//the language specific RelationUtilities::get()->isValidRelationEntityTypeCombo().  
	std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
	_observation = _new RelationObservation(validation_str.c_str());	

	_inst = _new PotentialRelationInstance();

	std::string vector_model_file = ParamReader::getRequiredParam("vector_relation_model_file");
	RelationTypeSet::initialize();
	_vectorModel = _new VectorModel(vector_model_file.c_str());

	std::string exec_file = ParamReader::getParam("exec_head_file");
	if (!exec_file.empty()) {
		_execTable = _new SymbolHash(exec_file.c_str());
	} else _execTable = 0;

	std::string staff_file = ParamReader::getParam("staff_head_file");
	if (!staff_file.empty()) {
		_staffTable = _new SymbolHash(staff_file.c_str());
	} else _staffTable = 0;

}

ChineseMaxEntRelationFinder::~ChineseMaxEntRelationFinder() {
	delete _observation;
	delete _decoder;
	delete _vectorModel;
}


void ChineseMaxEntRelationFinder::resetForNewSentence() {
	_n_relations = 0;
}

RelMentionSet *ChineseMaxEntRelationFinder::getRelationTheory(EntitySet *entitySet,
											   const Parse *parse,
											   MentionSet *mentionSet,
											   const ValueMentionSet *valueMentionSet,
											   PropositionSet *propSet,
											   const Parse *secondaryParse)
{
	_n_relations = 0;
	_currentSentenceIndex = mentionSet->getSentenceNumber();
	propSet->fillDefinitionsArray();

	_observation->resetForNewSentence(entitySet, parse, mentionSet, valueMentionSet, propSet);
	if (_decoder->DEBUG)
		_decoder->_debugStream << L"Sentence Number: " << _currentSentenceIndex << "\n\n";
	int nmentions = mentionSet->getNMentions();

	for (int i = 0; i < nmentions; i++) {
		if (!mentionSet->getMention(i)->isOfRecognizedType() ||
			mentionSet->getMention(i)->getMentionType() == Mention::NONE ||
			mentionSet->getMention(i)->getMentionType() == Mention::APPO ||
			mentionSet->getMention(i)->getMentionType() == Mention::LIST)
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			if (!mentionSet->getMention(j)->isOfRecognizedType() ||
				mentionSet->getMention(j)->getMentionType() == Mention::NONE ||
				mentionSet->getMention(j)->getMentionType() == Mention::APPO ||
				mentionSet->getMention(j)->getMentionType() == Mention::LIST)
				continue;
			if (!RelationUtilities::get()->validRelationArgs(mentionSet->getMention(i), mentionSet->getMention(j)))
				continue;
			_observation->populate(i, j);

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L" * " << mentionSet->getMention(i)->getUID() << " " << mentionSet->getMention(i)->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << mentionSet->getMention(i)->getNode()->toPrettyParse(3) << L"\n";
				_decoder->_debugStream << L" * " << mentionSet->getMention(j)->getUID() << " " << mentionSet->getMention(j)->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << mentionSet->getMention(j)->getNode()->toPrettyParse(3) << L"\n";
			}

			Symbol maxEntAnswer = _decoder->decodeToSymbol(_observation);
			Symbol vectorAnswer = _tagSet->getNoneTag();

			RelationPropLink *link = _observation->getPropLink();
			_inst->setStandardInstance(_observation);
			if (!link->isEmpty() && !link->isNegative()) {
				vectorAnswer
					= RelationTypeSet::getRelationSymbol(_vectorModel->findBestRelationType(_inst));
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"VECTOR: " << vectorAnswer.to_debug_string() << "\n";
				_decoder->_debugStream << L"MAXENT: " << maxEntAnswer.to_debug_string() << "\n";

				if (maxEntAnswer == _tagSet->getNoneTag() &&
					vectorAnswer != _tagSet->getNoneTag())
				{
					_decoder->_debugStream << L"V says yes; MAXENT says no\n";
				}

			}

			Symbol answer = maxEntAnswer;
			if (maxEntAnswer == _tagSet->getNoneTag() &&
				vectorAnswer != _tagSet->getNoneTag())
			{
				answer = vectorAnswer;
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"\n";
			}

			if (answer != _tagSet->getNoneTag()) {
				bool reverse = false;
				int int_answer = RelationTypeSet::getTypeFromSymbol(answer);
				if (!RelationTypeSet::isSymmetric(int_answer)) {
					_inst->setRelationType(answer);
					float non_reversed_score = _vectorModel->lookupB2P(_inst);

					int rev_int_answer = RelationTypeSet::reverse(int_answer);
					_inst->setRelationType(RelationTypeSet::getRelationSymbol(rev_int_answer));
					float reversed_score = _vectorModel->lookupB2P(_inst);

					if (_decoder->DEBUG) {
						_decoder->_debugStream << L"Reversed: " << reversed_score << "\n";
						_decoder->_debugStream << L"Not reversed: " << non_reversed_score << "\n";
					}

					if (non_reversed_score < reversed_score)
						reverse = true;
					if (_decoder->DEBUG && reverse) {
						_decoder->_debugStream << L"SO... reverse\n";
					}

				}
				if (reverse)
					addRelation(_observation->getMention2(), _observation->getMention1(), answer);
				else addRelation(_observation->getMention1(), _observation->getMention2(), answer);
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"\n";
			}

		}
	}

	RelMentionSet *result = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		result->takeRelMention(_relations[j]);
		_relations[j] = 0;
	}

	return result;
}


void ChineseMaxEntRelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type) {
	Symbol EMP_ORG_EXEC(L"EMP-ORG.Employ-Executive");
	Symbol EMP_ORG_STAFF(L"EMP-ORG.Employ-Staff");
	Symbol GPE_AFF(L"GPE-AFF");

	if (RelationConstants::getBaseTypeSymbol(type) == GPE_AFF) {
		if (_execTable != 0) {
			if (_execTable->lookup(first->getNode()->getHeadWord()))
				type = EMP_ORG_EXEC;
			else if (_execTable->lookup(second->getNode()->getHeadWord())) {
				type = EMP_ORG_EXEC;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
		if (_staffTable != 0) {
			if (_staffTable->lookup(first->getNode()->getHeadWord()))
				type = EMP_ORG_STAFF;
			else if (_staffTable->lookup(second->getNode()->getHeadWord())) {
				type = EMP_ORG_STAFF;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
	}
	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, 0);
		_n_relations++;
	}

}
