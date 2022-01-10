// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/relations/discmodel/en_P1RelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "English/wordnet/en_WordNet.h"

#include "Generic/disctagger/P1Decoder.h"
#include "Generic/disctagger/DTFeatureTypeSet.h"
#include "Generic/disctagger/DTTagSet.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/VectorModel.h"
#include "English/relations/en_TreeModel.h"
#include <boost/scoped_ptr.hpp>

UTF8OutputStream EnglishP1RelationFinder::_debugStream;
bool EnglishP1RelationFinder::DEBUG = false;

EnglishP1RelationFinder::EnglishP1RelationFinder() {

	P1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	char tag_set_file[500];
	ParamReader::getRequiredParam("relation_tag_set_file",tag_set_file,500);
	_tagSet = _new DTTagSet(tag_set_file, false, false);

	char features_file[500];
	ParamReader::getRequiredParam("relation_features_file",features_file,500);
	_featureTypes = _new DTFeatureTypeSet(features_file, P1RelationFeatureType::modeltype);

	char model_file[500];
	ParamReader::getRequiredParam("relation_model_file",model_file,500);

	// OVERGENERATION PERCENTAGE
	_overgen_percentage = ParamReader::getRequiredFloatParam("p1_relation_overgen_percentage");

	_weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_weights, model_file, P1RelationFeatureType::modeltype);

	_decoder = _new P1Decoder(_tagSet, _featureTypes, _weights, _overgen_percentage);

	//mrf - The relation validation string is used to determine which 
	//relation-type/argument types the decoder allows.  RelationObservation calls 
	//the language specific RelationUtilities::isValidRelationEntityTypeCombo(). 
	char validation_str[500];
	ParamReader::getRequiredParam("p1_relation_validation_str", validation_str, 500);
	_observation = _new RelationObservation(validation_str);

	_inst = _new PotentialRelationInstance();

	ParamReader::getRequiredParam("vector_tree_relation_model_file",model_file,500);
	RelationTypeSet::initialize();
	_vectorModel = _new VectorModel(model_file);
	_treeModel = _new TreeModel(model_file);

	_facOrgWords = _new SymbolHash(100);
	char parameter[500];
	if (ParamReader::getParam("ambiguous_fac_org_words",parameter,500))	{
		loadSymbolHash(_facOrgWords, parameter);
	}

}

EnglishP1RelationFinder::~EnglishP1RelationFinder() {
	delete _observation;
	delete _decoder;
	delete _facOrgWords;
	delete _vectorModel;
	delete _treeModel;
	delete _weights;
	delete _featureTypes;
	delete _tagSet;
	delete _inst;
}


void EnglishP1RelationFinder::resetForNewSentence() {
	_n_relations = 0;
}

RelMentionSet *EnglishP1RelationFinder::getRelationTheory(const Parse *parse,
											   MentionSet *mentionSet,
											   PropositionSet *propSet)
{
	_n_relations = 0;
	_currentSentenceIndex = mentionSet->getSentenceNumber();
	propSet->fillDefinitionsArray();

	_observation->resetForNewSentence(parse, mentionSet, propSet);
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

			if (!mentionSet->getMention(i)->isOfRecognizedType() ||
				!mentionSet->getMention(j)->isOfRecognizedType())
			{
				//tryForcingRelations(mentionSet, entitySet);
				continue;
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L" * " << mentionSet->getMention(i)->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << L" * " << mentionSet->getMention(j)->getNode()->toTextString() << L"\n";
			}

			Symbol discAnswer = _decoder->decodeToSymbol(_observation);
			Symbol vectorAnswer = _tagSet->getNoneTag();
			Symbol treeAnswer = _tagSet->getNoneTag();

			RelationPropLink *link = _observation->getPropLink();
			_inst->setStandardInstance(_observation);
			if (!link->isEmpty() && !link->isNegative()) {
				//vectorAnswer
				//	= RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
				vectorAnswer
					= RelationTypeSet::getRelationSymbol(_vectorModel->findBestRelationType(_inst));
				treeAnswer
					= RelationTypeSet::getRelationSymbol(_treeModel->findBestRelationType(_inst));
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"VECTOR: " << vectorAnswer.to_debug_string() << "\n";
				_decoder->_debugStream << L"TREE: " << treeAnswer.to_debug_string() << "\n";
				_decoder->_debugStream << L"P1: " << discAnswer.to_debug_string() << "\n";

				if (vectorAnswer == treeAnswer &&
					discAnswer != vectorAnswer &&
					vectorAnswer != _tagSet->getNoneTag())
				{
					_decoder->_debugStream << L"VT agree; P1 says no\n";
				}

			}

			Symbol answer = discAnswer;
			if (vectorAnswer == treeAnswer &&
				discAnswer != vectorAnswer &&
				vectorAnswer != _tagSet->getNoneTag())
			{
				answer = vectorAnswer;
			}

			if (answer == _tagSet->getNoneTag() &&
				vectorAnswer != _tagSet->getNoneTag())
			{
				answer = vectorAnswer;
			}

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"\n";
			}

			if (answer != _tagSet->getNoneTag()) {
				if (shouldBeReversed(answer))
					addRelation(_observation->getMention2(), _observation->getMention1(), answer);
				else addRelation(_observation->getMention1(), _observation->getMention2(), answer);
			} else tryForcingRelations(mentionSet);

		}
	}

	RelMentionSet *result = _new RelMentionSet(_n_relations);
	for (int j = 0; j < _n_relations; j++) {
		result->takeRelMention(j, _relations[j]);
		_relations[j] = 0;
	}

	return result;
}


void EnglishP1RelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type) {
	if (fixRelation(first, second, type))
		return;
	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, 0);
		_n_relations++;
	}
}

// THIS SHOULD GET CHANGED INTO A LIST. I AM TIRED AND NOT GOING TO DO IT RIGHT NOW.

static Symbol jewishSym = Symbol(L"jewish");
static Symbol westernSym = Symbol(L"western");
static Symbol republicanSym = Symbol(L"republican");
static Symbol christianSym = Symbol(L"christian");
static Symbol muslimSym = Symbol(L"muslim");
static Symbol democraticSym = Symbol(L"democratic");
static Symbol democratSym = Symbol(L"democrat");
static Symbol islamicSym = Symbol(L"islamic");
static Symbol shiiteSym = Symbol(L"shiite");
static Symbol sunniSym = Symbol(L"sunni");
static Symbol communistSym = Symbol(L"communist");
static Symbol sikhSym = Symbol(L"sikh");
static Symbol catholicSym = Symbol(L"catholic");
static Symbol hinduSym = Symbol(L"hindu");
static Symbol methodistSym = Symbol(L"methodist");
static Symbol neonaziSym = Symbol(L"neo-nazi");
static Symbol zionistSym = Symbol(L"zionist");
static Symbol arabSym = Symbol(L"arab");
static Symbol hispanicSym = Symbol(L"hispanic");
static Symbol africanAmericanSym = Symbol(L"african-american");
static Symbol persianSym = Symbol(L"persian");
static Symbol latinoSym = Symbol(L"latino");
static Symbol latinaSym = Symbol(L"latina");

bool EnglishP1RelationFinder::fixRelation(const Mention *first, const Mention *second, Symbol type) {
	if (second->getMentionType() == Mention::NAME) {
		Symbol headword = second->getNode()->getHeadWord();
		if (headword == jewishSym ||
			headword == westernSym)
		{
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Other")) ==
				RelationTypeSet::INVALID_TYPE)
				return false;

			if (_n_relations < MAX_SENTENCE_RELATIONS) {
				_relations[_n_relations] = _new RelMention(first, second,
					Symbol(L"OTHER-AFF.Other"), _currentSentenceIndex, _n_relations, 0);
				_n_relations++;
			}
			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"FORCED:\n";
				_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << type.to_string() << L"\n";
			}
			return true;
		} else if (headword == christianSym ||
			headword == muslimSym ||
			headword == islamicSym ||
			headword == shiiteSym ||
			headword == sunniSym ||
			headword == sikhSym ||
			headword == catholicSym ||
			headword == hinduSym ||
			headword == methodistSym ||
			headword == neonaziSym ||
			headword == zionistSym)
		{
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Ideology")) ==
				RelationTypeSet::INVALID_TYPE)
				return false;

			if (_n_relations < MAX_SENTENCE_RELATIONS) {
				_relations[_n_relations] = _new RelMention(first, second,
					Symbol(L"OTHER-AFF.Ideology"), _currentSentenceIndex, _n_relations, 0);
				_n_relations++;
			}
			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"FORCED:\n";
				_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << type.to_string() << L"\n";
			}
			return true;
		} else if (headword == arabSym ||
			headword == hispanicSym ||
			headword == africanAmericanSym ||
			headword == persianSym ||
			headword == latinoSym ||
			headword == latinaSym)
		{
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Ethnic")) ==
				RelationTypeSet::INVALID_TYPE)
				return false;

			if (_n_relations < MAX_SENTENCE_RELATIONS) {
				_relations[_n_relations] = _new RelMention(first, second,
					Symbol(L"OTHER-AFF.Ethnic"), _currentSentenceIndex, _n_relations, 0);
				_n_relations++;
			}
			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"FORCED:\n";
				_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << type.to_string() << L"\n";
			}
			return true;
		}
	}
	return false;
}


void EnglishP1RelationFinder::tryForcingRelations(MentionSet *mentionSet) {

	const Mention *m1 = _observation->getMention1();
	const Mention *m2 = _observation->getMention2();

	// at least one has to be of a known type!
	if (!m1->getEntityType().isRecognized() && !m2->getEntityType().isRecognized())
		return;

	// ought to know what we're doing at least a little to try this
	if (_observation->getPropLink()->isEmpty())
		return;

	Symbol acceptableTypes[4];
	acceptableTypes[0] = Symbol(L"PHYS.Located");
	acceptableTypes[1] = Symbol(L"PHYS.Near");
	acceptableTypes[2] = Symbol();
	acceptableTypes[3] = Symbol();

	const Mention *orgMention = 0;
	if (m1->getEntityType().matchesORG() && m2->getEntityType().matchesPER()) {
		if (_facOrgWords->lookup(m1->getNode()->getHeadWord()))
			orgMention = m1;
	} else if (m2->getEntityType().matchesORG() && m1->getEntityType().matchesPER()) {
		if (_facOrgWords->lookup(m2->getNode()->getHeadWord())) {
			orgMention = m2;
		}
	}

	if (orgMention != 0) {
		if (tryFakeTypeRelation(mentionSet, orgMention,
			EntityType::getLOCType(), acceptableTypes))
			return;
		if (tryFakeTypeRelation(mentionSet, orgMention,
			EntityType::getFACType(), acceptableTypes))
			return;
	}

	const Mention *facMention = 0;
	if (m1->getEntityType().matchesFAC() && m2->getEntityType().matchesPER()) {
		facMention = m1;
	} else if (m2->getEntityType().matchesFAC() && m1->getEntityType().matchesPER()) {
		facMention = m2;
	}

	if (facMention != 0) {
		if (tryFakeTypeRelation(mentionSet, facMention,
			EntityType::getLOCType(), acceptableTypes))
			return;
	}

	const Mention *othMention = 0;
	if (!m1->getEntityType().isRecognized())
		othMention = m1;
	if (!m2->getEntityType().isRecognized())
		othMention = m2;

	//if (othMention != 0) {
	//	if (WordNet::getInstance()->isPerson(othMention->getNode()->getHeadWord())) {
	//		if (tryCoercedTypeRelation(mentionSet, entitySet, othMention,
	//			EntityType::getPERType()))
	//			return;
	//	}
	//}

}
bool EnglishP1RelationFinder::tryFakeTypeRelation(MentionSet *mentionSet,
												 const Mention *possMention,
												 EntityType possibleType,
												 Symbol *acceptableTypes)
{
	EntityType origType = possMention->getEntityType();

	if (!possibleType.isRecognized() || !origType.isRecognized())
		return false;

	mentionSet->changeEntityType(possMention->getUID(), possibleType);
	Symbol answer = _decoder->decodeToSymbol(_observation);

	if (answer != _tagSet->getNoneTag() &&
		(answer == acceptableTypes[0] || answer == acceptableTypes[1] ||
		answer == acceptableTypes[2] || answer == acceptableTypes[3]))
	{
		//std::cerr << "FORCED (from ";
		//std::cerr << origType.getName().to_debug_string() << " to ";
		//std::cerr << possibleType.getName().to_debug_string() << "):\n";
		//std::cerr << _observation->getMention1()->getNode()->toDebugTextString() << "\n";
		//std::cerr << _observation->getMention2()->getNode()->toDebugTextString() << "\n";
		//std::cerr << answer.to_debug_string() << "\n";
		mentionSet->changeEntityType(possMention->getUID(), origType);

		if (shouldBeReversed(answer))
			addRelation(_observation->getMention2(), _observation->getMention1(), answer);
		else addRelation(_observation->getMention1(), _observation->getMention2(), answer);

		return true;
	} else {
		// if not, just change it back
		mentionSet->changeEntityType(possMention->getUID(), origType);
	}
	return false;
}

/*bool EnglishP1RelationFinder::tryCoercedTypeRelation(MentionSet *mentionSet, EntitySet *entitySet,
												 const Mention *possMention,
												 EntityType possibleType)
{
	EntityType origType = possMention->getEntityType();

	if (!possibleType.isRecognized())
		return false;

	std::cerr << "Trying: " << possMention->getNode()->toDebugTextString() << "\n";

	// let's see if it would be a relation
	mentionSet->changeEntityType(possMention->getUID(), possibleType);
	Symbol answer = _decoder->decodeToSymbol(_observation);

	if (answer == _tagSet->getNoneTag()) {
		mentionSet->changeEntityType(possMention->getUID(), origType);
		return false;
	}

	float answer_score = _decoder->lookupScore(_observation, answer);
	float none_score = _decoder->lookupScore(_observation, answer);
	float diff = answer_score - none_score;
	if (diff / answer_score < .5) {
		std::cerr << "Relation not sure\n";
		mentionSet->changeEntityType(possMention->getUID(), origType);
		return false;
	}

	std::cerr << "FORCED (from ";
	std::cerr << origType.getName().to_debug_string() << " to ";
	std::cerr << possibleType.getName().to_debug_string() << "):\n";
	std::cerr << _observation->getMention1()->getNode()->toDebugTextString() << "\n";
	std::cerr << _observation->getMention2()->getNode()->toDebugTextString() << "\n";
	std::cerr << answer.to_debug_string() << "\n";

	entitySet->getNonConstLastMentionSet()->changeEntityType(possMention->getUID(),
		possibleType);
	entitySet->addNew(possMention->getUID(), possibleType);

	if (shouldBeReversed(answer))
		addRelation(_observation->getMention2(), _observation->getMention1(), answer);
	else addRelation(_observation->getMention1(), _observation->getMention2(), answer);

	return true;
}
*/


bool EnglishP1RelationFinder::shouldBeReversed(Symbol answer) {

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
	return reverse;
}

void EnglishP1RelationFinder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		string err = "problem opening ";
		err.append(file);
		throw UnexpectedInputException("EnglishP1RelationFinder::loadSymbolHash()",
			(char *)err.c_str());
	}

	wchar_t line[501];
	while (!stream.eof()) {
		stream.getLine(line, 500);
		if (line[0] == L'#')
			continue;
		wcslwr(line);
		Symbol lineSym(line);
		hash->add(lineSym);
	}

	stream.close();
}

