// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_ComboRelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"

#include "Generic/common/ParamReader.h"
#include "Chinese/common/ch_WordConstants.h"
#include "Generic/common/SymbolHash.h"

#include "Generic/discTagger/P1Decoder.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Chinese/relations/discmodel/ch_P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/VectorModel.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

UTF8OutputStream ChineseComboRelationFinder::_debugStream;
bool ChineseComboRelationFinder::DEBUG = false;

Symbol ChineseComboRelationFinder::NO_RELATION_SYM = Symbol(L"NO_RELATION");

ChineseComboRelationFinder::ChineseComboRelationFinder() : _allow_mention_set_changes(false) {

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
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);

	std::string p1_model_file = ParamReader::getParam("p1_relation_model_file");
	if (!p1_model_file.empty()) {
        // OVERGENERATION PERCENTAGE
		double overgen_percentage = ParamReader::getRequiredFloatParam("p1_relation_overgen_percentage");

		_p1Weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1Weights, p1_model_file.c_str(), P1RelationFeatureType::modeltype);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, overgen_percentage);
	} else {
		_p1Decoder = 0;
		_p1Weights = 0;
	}

	std::string maxent_model_file = ParamReader::getParam("maxent_relation_model_file");	
	if (!maxent_model_file.empty()) {
		_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxentWeights, maxent_model_file.c_str(), P1RelationFeatureType::modeltype);

		_maxentDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
	} else {
		_maxentDecoder = 0;
		_maxentWeights = 0;
	}

	std::string secondary_model_file = ParamReader::getParam("secondary_p1_relation_model_file");		
	if (!secondary_model_file.empty()) {
   		_p1SecondaryWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1SecondaryWeights, secondary_model_file.c_str(), P1RelationFeatureType::modeltype);

		std::string secondary_features_files = ParamReader::getRequiredParam("secondary_p1_relation_features_file");
		_p1SecondaryFeatureTypes = _new DTFeatureTypeSet(secondary_features_files.c_str(), P1RelationFeatureType::modeltype);

		_p1SecondaryDecoder = _new P1Decoder(_tagSet, _p1SecondaryFeatureTypes, _p1SecondaryWeights, 0, true);
	} else {
		_p1SecondaryDecoder = 0;
		_p1SecondaryFeatureTypes = 0;
		_p1SecondaryWeights = 0;
	}

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

ChineseComboRelationFinder::~ChineseComboRelationFinder() {
	delete _observation;
	delete _p1Decoder;
	delete _p1SecondaryDecoder;
	delete _maxentDecoder;
	delete _vectorModel;
	delete _p1Weights;
	delete _p1SecondaryWeights;
	delete _maxentWeights;
	delete _featureTypes;
	delete _p1SecondaryFeatureTypes;
	delete _tagSet;
	delete _inst;
}


void ChineseComboRelationFinder::resetForNewSentence() {
	_n_relations = 0;
}

RelMentionSet *ChineseComboRelationFinder::getRelationTheory(EntitySet *entitySet,
											   const Parse *parse,
											   MentionSet *mentionSet,
											   ValueMentionSet *valueMentionSet,
											   PropositionSet *propSet)
{
	_n_relations = 0;
	_currentSentenceIndex = mentionSet->getSentenceNumber();
	propSet->fillDefinitionsArray();

	_observation->resetForNewSentence(entitySet, parse, mentionSet, valueMentionSet, propSet);
	int nmentions = mentionSet->getNMentions();

	for (int i = 0; i < nmentions; i++) {
		if (mentionSet->getMention(i)->getMentionType() == Mention::NONE ||
			mentionSet->getMention(i)->getMentionType() == Mention::APPO ||
			mentionSet->getMention(i)->getMentionType() == Mention::LIST)
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			if (mentionSet->getMention(j)->getMentionType() == Mention::NONE ||
				mentionSet->getMention(j)->getMentionType() == Mention::APPO ||
				mentionSet->getMention(j)->getMentionType() == Mention::LIST)
				continue;
			_observation->populate(i, j);

			// this means we're doing sentence-level relation finding
			// currently the only implemented use of this is to find mention set changes
			if (_allow_mention_set_changes) {
				findMentionSetChanges(mentionSet);
				continue;
			}			

			if (!mentionSet->getMention(i)->isOfRecognizedType() ||
				!mentionSet->getMention(j)->isOfRecognizedType())
			{
				continue;
			}

			if (DEBUG) {
				_debugStream << L" * " << mentionSet->getMention(i)->getNode()->toTextString() << L"\n";
				_debugStream << L" * " << mentionSet->getMention(j)->getNode()->toTextString() << L"\n";
			}

			Symbol p1Answer = _tagSet->getNoneTag();
			Symbol maxentAnswer = _tagSet->getNoneTag();
			Symbol confidentVectorAnswer = _tagSet->getNoneTag();
			Symbol vectorAnswer = _tagSet->getNoneTag();
			Symbol secondaryAnswer = _tagSet->getNoneTag();

			_inst->setStandardInstance(_observation);
			Symbol specialCaseAnswer = findSpecialCaseRelation(mentionSet);
			if (_tagSet->getTagIndex(specialCaseAnswer) == -1)
				specialCaseAnswer = _tagSet->getNoneTag();

			if (specialCaseAnswer == NO_RELATION_SYM)
				continue;

			RelationPropLink *link = _observation->getPropLink();
			if (!link->isEmpty() && !link->isNegative()) {
				confidentVectorAnswer
					= RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
				vectorAnswer
					= RelationTypeSet::getRelationSymbol(_vectorModel->findBestRelationType(_inst));
			}

			if (_p1Decoder)
				p1Answer = _p1Decoder->decodeToSymbol(_observation);

			if (_maxentDecoder)
				maxentAnswer = _maxentDecoder->decodeToSymbol(_observation);

			if (_p1SecondaryDecoder)
				secondaryAnswer = _p1SecondaryDecoder->decodeToSymbol(_observation);

			if (DEBUG) {
				_debugStream << L"VECTOR: " << vectorAnswer.to_debug_string() << "\n";
				_debugStream << L"CONFIDENT VECTOR: " << confidentVectorAnswer.to_debug_string() << "\n";
				if (_p1Decoder) {
					_debugStream << L"P1: " << p1Answer.to_debug_string() << "\n";
					if (p1Answer != _tagSet->getNoneTag())
						_p1Decoder->printDebugInfo(_observation, _tagSet->getNoneTagIndex(), _debugStream);
					_p1Decoder->printDebugInfo(_observation, _tagSet->getTagIndex(p1Answer), _debugStream);
				}
				if (_maxentDecoder) {
					_debugStream << L"MAXENT: " << maxentAnswer.to_debug_string() << "\n";
				}
				if (_p1SecondaryDecoder) {
					_debugStream << L"SECONDARY P1: " << secondaryAnswer.to_debug_string() << "\n";							
				}
			}

			// combine these in some clever way
			Symbol answer = specialCaseAnswer;		

			if (answer == _tagSet->getNoneTag())
				answer = p1Answer;

			if (answer == _tagSet->getNoneTag())
				answer = maxentAnswer;

			if (answer == _tagSet->getNoneTag())
				answer = confidentVectorAnswer;

			// If all are non-none, go with the most popular answer
			if (p1Answer != _tagSet->getNoneTag() && maxentAnswer != _tagSet->getNoneTag() &&
				confidentVectorAnswer != _tagSet->getNoneTag())
			{
				if (p1Answer == maxentAnswer) 
					answer = p1Answer;
				else if (p1Answer == confidentVectorAnswer)
					answer = p1Answer;
				else if (maxentAnswer == confidentVectorAnswer) 
					answer = maxentAnswer;
			}
			// if the maxent answer and p1 answers disagree, go with the one
			// supported by the vector answer
			else if (maxentAnswer != _tagSet->getNoneTag() &&
				     p1Answer != _tagSet->getNoneTag() &&
					 maxentAnswer != p1Answer)
			{ 
				if (vectorAnswer == p1Answer)
					answer = p1Answer;
				else if (vectorAnswer == maxentAnswer)
					answer = maxentAnswer;
			}

			if (answer == _tagSet->getNoneTag() && !link->isEmpty()) {
				answer = secondaryAnswer;
				if (answer != _tagSet->getNoneTag() && DEBUG) {
					_debugStream << L"SECONDARY SELECTED: " << secondaryAnswer.to_debug_string() << "\n";
					_p1SecondaryDecoder->printDebugInfo(_observation, _tagSet->getNoneTagIndex(), _debugStream);
					_p1SecondaryDecoder->printDebugInfo(_observation, _tagSet->getTagIndex(secondaryAnswer), _debugStream);
				}
			}

			if (DEBUG) {
				_debugStream << L"\n";
			}

			if (answer != _tagSet->getNoneTag()) {
				if (shouldBeReversed(answer))
					addRelation(_observation->getMention2(), _observation->getMention1(), answer);
				else addRelation(_observation->getMention1(), _observation->getMention2(), answer);
			} else tryForcingRelations(mentionSet);

		}
	}

	int killed_rel_mentions = 0;
	
	/*	THIS WAS ORIGINALLY IMPLEMENTED IN ENGLISH AND I'M HESITANT TO LEAVE
		IT TURNED ON FOR CHINESE.

	// What can we do about situations like "US military officials"
	// The relation finder will likely find (US, officials), (US, military), and (military, officials)
	// This is not really a great idea -- this solution is overkill, but oh well.
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition *prop = propSet->getProposition(p);
		if (prop->getNArgs() < 3 || prop->getPredType() != Proposition::NOUN_PRED)
			continue;
		const Mention *refMent = 0;
		const Mention *firstPremodMent = 0;
		const Mention *secondPremodMent = 0;
		for (int a = 0; a < prop->getNArgs(); a++) {
			if (prop->getArg(a)->getRoleSym() == Argument::REF_ROLE)
				refMent = prop->getArg(a)->getMention(mentionSet);
			if (prop->getArg(a)->getRoleSym() == Argument::UNKNOWN_ROLE && firstPremodMent == 0)
				firstPremodMent = prop->getArg(a)->getMention(mentionSet);
			else if (prop->getArg(a)->getRoleSym() == Argument::UNKNOWN_ROLE &&	secondPremodMent == 0)
				secondPremodMent = prop->getArg(a)->getMention(mentionSet);
		}
		if (secondPremodMent == 0 || firstPremodMent == 0 || refMent == 0)
			continue;
		int ref_first = -1;
		int ref_second = -1;
		int first_second = -1;
		for (int j = 0; j < _n_relations; j++) {
			if (_relations[j] == 0)
				continue;
			if (_relations[j]->getLeftMention() == refMent) {
				if (_relations[j]->getRightMention() == firstPremodMent)
					ref_first = j;
				if (_relations[j]->getRightMention() == secondPremodMent)
					ref_second = j;
			} else if (_relations[j]->getLeftMention() == firstPremodMent) {
				if (_relations[j]->getRightMention() == refMent)
					ref_first = j;
				if (_relations[j]->getRightMention() == secondPremodMent)
					first_second = j;
			} else if (_relations[j]->getLeftMention() == secondPremodMent) {
				if (_relations[j]->getRightMention() == firstPremodMent)
					first_second = j;
				if (_relations[j]->getRightMention() == refMent)
					ref_second = j;
			} 
		}
		if (ref_first == -1 || ref_second == -1 || first_second == -1)
			continue;
		if (firstPremodMent->getIndex() < secondPremodMent->getIndex()) {
			killed_rel_mentions++;
			delete _relations[ref_first];
			_relations[ref_first] = 0;
		} else {
			killed_rel_mentions++;
			delete _relations[ref_second];
			_relations[ref_second] = 0;
		}
	}
	*/


	RelMentionSet *result = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		if (_relations[j] != 0)
			result->takeRelMention(_relations[j]);
		_relations[j] = 0;
	}

	return result;
}

Symbol ChineseComboRelationFinder::findSpecialCaseRelation(MentionSet *mentionSet) {

	/*
	RelationPropLink *propLink = _observation->getPropLink();

	// sign-offs
	if (!propLink->isEmpty() && propLink->getTopProposition()->getPredType() == Proposition::SET_PRED &&		
		propLink->getArgument1()->getRoleSym() == Argument::MEMBER_ROLE &&
		propLink->getArgument2()->getRoleSym() == Argument::MEMBER_ROLE)
	{
		Proposition *prop = propLink->getTopProposition();
		if (prop->getNArgs() >= 4 &&
			prop->getArg(0)->getType() == Argument::MENTION_ARG &&
			prop->getArg(1)->getType() == Argument::MENTION_ARG &&
			prop->getArg(2)->getType() == Argument::MENTION_ARG &&
			prop->getArg(3)->getType() == Argument::MENTION_ARG) 
		{
			const Mention *person = prop->getArg(1)->getMention(mentionSet);
			const Mention *organization = prop->getArg(2)->getMention(mentionSet);
			const Mention *location = prop->getArg(3)->getMention(mentionSet);
			if (person->getEntityType().matchesPER() &&
				organization->getEntityType().matchesORG() &&
				(location->getEntityType().matchesGPE() ||
				location->getEntityType().matchesFAC() ||
				location->getEntityType().matchesLOC()))
			{
				if ((_observation->getMention1() == person && 
					_observation->getMention2() == organization) ||
					(_observation->getMention2() == person &&
					_observation->getMention1() == organization))
				{
					return SpecialRelationCases::getEmployeesType();
				} else if ((_observation->getMention1() == person && 
					_observation->getMention2() == location) ||
					(_observation->getMention2() == person &&
					_observation->getMention1() == location))
				{
					return SpecialRelationCases::getLocatedType();
				} else return NO_RELATION_SYM;
			}
		}
	}
	*/
	return _tagSet->getNoneTag();

}


void ChineseComboRelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type) {
	if (fixRelation(first, second, type))
		return;
	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, 0);
		_n_relations++;
	}
}

bool ChineseComboRelationFinder::fixRelation(const Mention *first, const Mention *second, Symbol type) {

	Symbol forcedType = Symbol();

	Symbol EMP_ORG_EXEC(L"EMP-ORG.Employ-Executive");
	Symbol EMP_ORG_STAFF(L"EMP-ORG.Employ-Staff");
	Symbol GPE_AFF(L"GPE-AFF");

	// this all applies only to ACE2004
	if (RelationConstants::getBaseTypeSymbol(type) == GPE_AFF) {
		if (_execTable != 0) {
			if (_execTable->lookup(first->getNode()->getHeadWord()))
				forcedType = EMP_ORG_EXEC;
			else if (_execTable->lookup(second->getNode()->getHeadWord())) {
				forcedType = EMP_ORG_EXEC;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
		if (_staffTable != 0) {
			if (_staffTable->lookup(first->getNode()->getHeadWord()))
				forcedType = EMP_ORG_STAFF;
			else if (_staffTable->lookup(second->getNode()->getHeadWord())) {
				forcedType = EMP_ORG_STAFF;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
	}

	if (!forcedType.is_null()) {
		if (_n_relations < MAX_SENTENCE_RELATIONS) {
			_relations[_n_relations] = _new RelMention(first, second,
				forcedType, _currentSentenceIndex, _n_relations, 0);
			_n_relations++;
		}
		if (DEBUG) {
			_debugStream << L"FORCED:\n";
			_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
			_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
			_debugStream << forcedType.to_string() << L"\n";
		}
		return true;
	}

	return false;
}

void ChineseComboRelationFinder::findMentionSetChanges(MentionSet *mentionSet) {
	const Mention *m1 = _observation->getMention1();
	const Mention *m2 = _observation->getMention2();

	const Mention *knownMention = 0;
	const Mention *unknownMention = 0;
	if (m1->getEntityType().isRecognized() && !m2->getEntityType().isRecognized()) {
		knownMention = m1;
		unknownMention = m2;
	} else if (m2->getEntityType().isRecognized() && !m1->getEntityType().isRecognized()) {
		knownMention = m2;
		unknownMention = m1;
	}

	// if there's no proplink, we don't know nearly enough to be doing this kind of thing
	if (_observation->getPropLink()->isEmpty() || _observation->getPropLink()->isNegative())
		return;

	// look at potential people pronouns
	if (unknownMention != 0) {
		Symbol headword = unknownMention->getNode()->getHeadWord();
		if (unknownMention->getMentionType() == Mention::PRON &&
			(headword == ChineseWordConstants::THEIR_FEM ||
			headword == ChineseWordConstants::THEIR_MASC ||
			headword == ChineseWordConstants::THEIR_INANIMATE ||
			headword == ChineseWordConstants::THEY_FEM ||
			headword == ChineseWordConstants::THEY_MASC ||
			headword == ChineseWordConstants::THEY_INANIMATE ||
			headword == ChineseWordConstants::WHO))
		{
			mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getPERType());
			_inst->setStandardInstance(_observation);		
			Symbol answer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			if (answer == Symbol(L"PER-SOC.Family") ||
				answer == Symbol(L"PER-SOC.Business") ||
				answer == Symbol(L"PER-SOC.Lasting-Personal") ||
				answer == Symbol(L"ORG-AFF.Employment"))
			{			
				std::cerr << "Forcing pronoun (" << headword << ") to PER due to relation:\n";
				std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
			} else mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getUndetType());
		}
	} 

	const Mention *pronMention = 0;
	const Mention *otherMention = 0;
	if (m1->getMentionType() == Mention::PRON && m2->getMentionType() != Mention::PRON) {
		pronMention = m1;
		otherMention = m2;
	} else if (m2->getMentionType() == Mention::PRON && m1->getMentionType() != Mention::PRON) {
		pronMention = m2;
		otherMention = m1;
	} 
		
	if (pronMention != 0 && pronMention->getEntityType().matchesPER()) {
		Symbol headword = pronMention->getNode()->getHeadWord();
		if (headword == ChineseWordConstants::WE ||
			headword == ChineseWordConstants::OUR)
		{
			std::cerr << "Found 1p plural pronoun\n";
			if (pronMention->getNode()->getParent() != 0 &&
				pronMention->getNode()->getParent()->getParent() != 0)
			{
				std::cerr << pronMention->getNode()->getParent()->getParent()->toDebugTextString() << "\n";
			}
			// should start as PER type
			_inst->setStandardInstance(_observation);
			float per_score = -10000;
			float org_score = -10000;
			float gpe_score = -10000;
			Symbol perAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			std::cerr << "PER -- " << perAnswer << "\n";
			if (perAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(perAnswer);
				per_score = _vectorModel->lookup(_inst);
			}
			mentionSet->changeEntityType(pronMention->getUID(), EntityType::getORGType());
			_inst->setStandardInstance(_observation);
			Symbol orgAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			if (orgAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(orgAnswer);
				org_score = _vectorModel->lookup(_inst);
				std::cerr << "ORG -- " << orgAnswer << " " << org_score << "\n";				
			}
			mentionSet->changeEntityType(pronMention->getUID(), EntityType::getGPEType());
			_inst->setStandardInstance(_observation);
			Symbol gpeAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			std::cerr << "GPE -- " << gpeAnswer << "\n";
			if (gpeAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(gpeAnswer);
				gpe_score = _vectorModel->lookup(_inst);
			}

			EntityType etype = EntityType::getPERType();
			if (org_score != -10000 && org_score >= per_score && org_score >= gpe_score) {
				etype = EntityType::getORGType();
			} else if (gpe_score != -10000 && gpe_score >= per_score && gpe_score >= org_score) {
				etype = EntityType::getGPEType();
			} 
			if (!etype.matchesPER()) {
				std::cerr << "Forcing pronoun (" << headword << ") to " << etype.getName() << " due to relation:\n";
				std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
				mentionSet->changeEntityType(pronMention->getUID(), etype);
			} else mentionSet->changeEntityType(pronMention->getUID(), EntityType::getPERType());
		} 
	}

}

// I'm not going to implement this for Chinese right now...
void ChineseComboRelationFinder::tryForcingRelations(MentionSet *mentionSet) {

	/*const Mention *m1 = _observation->getMention1();
	const Mention *m2 = _observation->getMention2();

	// at least one has to be of a known type!
	if (!m1->getEntityType().isRecognized() && !m2->getEntityType().isRecognized())
		return;

	// ought to know what we're doing at least a little to try this
	if (_observation->getPropLink()->isEmpty())
		return;
	*/

	return;

}


bool ChineseComboRelationFinder::shouldBeReversed(Symbol answer) {

	bool reverse = false;
	int int_answer = RelationTypeSet::getTypeFromSymbol(answer);
	if (!RelationTypeSet::isSymmetric(int_answer)) {
		_inst->setRelationType(answer);
		float non_reversed_score = _vectorModel->lookupB2P(_inst);

		int rev_int_answer = RelationTypeSet::reverse(int_answer);
		_inst->setRelationType(RelationTypeSet::getRelationSymbol(rev_int_answer));
		float reversed_score = _vectorModel->lookupB2P(_inst);

		if (DEBUG) {
			_debugStream << L"Reversed: " << reversed_score << "\n";
			_debugStream << L"Not reversed: " << non_reversed_score << "\n";
		}

		if (non_reversed_score < reversed_score)
			reverse = true;
		if (DEBUG && reverse) {
			_debugStream << L"SO... reverse\n";
		}
	}
	return reverse;
}

void ChineseComboRelationFinder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		string err = "Problem opening ";
		err.append(file);
		throw UnexpectedInputException("ChineseComboRelationFinder::loadSymbolHash()",
			(char *)err.c_str());
	}

	std::wstring line;
	while (!stream.eof()) {
		stream.getLine(line);
		if (line.size() == 0 || line.at(0) == L'#')
			continue;
		std::transform(line.begin(), line.end(), line.begin(), towlower);
		Symbol lineSym(line.c_str());
		hash->add(lineSym);
	}

	stream.close();
}

