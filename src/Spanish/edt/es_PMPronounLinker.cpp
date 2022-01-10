// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/edt/es_PMPronounLinker.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SymbolConstants.h"
//#include "Spanish/common/es_WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/Guesser.h"
#include "Generic/edt/PronounLinkerUtils.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/EntityType.h"

#include <string.h>
#include <math.h>
#include <boost/scoped_ptr.hpp>

DebugStream &SpanishPMPronounLinker::_debugStream = SpanishPronounLinker::getDebugStream();

SpanishPMPronounLinker::SpanishPMPronounLinker() {
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	//initialize 4 ProbModels
	std::string model_prefix = ParamReader::getRequiredParam("pronlink_model");
	_initialize(model_prefix.c_str());
	HobbsDistance::initialize();
}

SpanishPMPronounLinker::~SpanishPMPronounLinker() {
	delete _priorAntModel;
	delete _hobbsDistanceModel;
	delete _pronounGivenAntModel;
	delete _parWordGivenAntModel;
}

void SpanishPMPronounLinker::addPreviousParse(const Parse *parse) {
	_previousParses.push_back(parse);
}

void SpanishPMPronounLinker::resetPreviousParses() {
	_previousParses.clear();
}

int SpanishPMPronounLinker::linkMention (LexEntitySet * currSolution,
								MentionUID currMentionUID,
								EntityType linkType,
								LexEntitySet *results[],
								int max_results)
{
	//run getLinkGuesses() to get a scored list
	//of link possibilities, create new LexEntitySet's
	Mention *currMention = currSolution->getMention(currMentionUID);
	const SynNode *pronounNode = currMention->node;
	LinkGuess linkGuesses[64];

	max_results = (max_results > 64) ? 64 : max_results;

	if (_use_correct_answers) {
		// if we are using correct mentions, we only want to link pronouns with either
		// a recognized type (totally unfair) or, 
		// as in the EDR conditional eval, with type UNDET.
		if (currMention->getEntityType() == EntityType::getOtherType()) {
			results[0] = currSolution->fork();
			return 1;
		}
	}

	int nGuesses = getLinkGuesses(currSolution, currMention, linkGuesses, max_results);

	int i, nResults = 0;
	bool alreadyProcessedNullDecision = false;
	//these will be used to determine entity linking for this pronoun
	const SynNode *guessedNode;
	Mention *guessedMention;
	Entity *guessedEntity;

	_debugStream << L"-------------------------------\n";
	_debugStream << "RESULTS:\n";

	for (i = 0; i < nGuesses; i++) {
		guessedNode = linkGuesses[i].guess;

		//determine mention from node
		if (guessedNode != NULL) {
			guessedMention = currSolution->getMentionSet(linkGuesses[i].sentence_num)
				->getMentionByNode(guessedNode);
			if (guessedMention->getMentionType() == Mention::NONE) {
				const SynNode *parent = guessedMention->getNode()->getParent();
				Mention *parentMent = currSolution->getMentionSet(linkGuesses[i].sentence_num)
					->getMentionByNode(parent);
				while (parentMent && parentMent->getMentionType() == Mention::NONE) {
					parent = parent->getParent();
					parentMent = currSolution->getMentionSet(linkGuesses[i].sentence_num)
									->getMentionByNode(parent);
				}
				if (parentMent) {
					guessedMention = parentMent;
					_debugStream << "NONE mention - replaced with parent\n";
				}
			}
		}
		else guessedMention = NULL;

		//determine Entity from Mention
		if (guessedMention != NULL) 
			guessedEntity = currSolution->getEntityByMention(guessedMention->getUID());
		else guessedEntity = NULL;

		//many different pronoun resolution decisions can lead to a "no-link" result.
		//therefore, only fork the LexEntitySet for the top scoring no-link guess.
		if (guessedEntity != NULL || !alreadyProcessedNullDecision) {
			LexEntitySet *newSet = currSolution->fork();

			if (guessedMention != NULL) {
				newSet->getMention(currMention->getUID())->setEntityType(guessedMention->getEntityType());
				_debugStream << "CHOSE mention " << guessedMention->getUID();
				_debugStream << " entity type " << guessedMention->getEntityType().getName().to_string();
				_debugStream << " mention type " << guessedMention->getMentionType() << "\n";
			} else {
				_debugStream << "NULL mention\n";
			}

			if (guessedEntity == NULL) {
				alreadyProcessedNullDecision = true;
				// for pronouns we gave types to in relation finding, say
				if (currMention->getEntityType().isRecognized()) {
					newSet->getMention(currMention->getUID())->setEntityType(currMention->getEntityType());				
					newSet->addNew(currMention->getUID(), currMention->getEntityType());
					_debugStream << "Forcing pronoun to be an entity (perhaps because it's in a relation OR because we've already classified pronouns): ";
					if (currMention->getNode()->getParent() != 0 && 
						currMention->getNode()->getParent()->getParent() != 0)
					{
						_debugStream << currMention->getNode()->getParent()->getParent()->toTextString();
					}
					_debugStream << "\n";
				} else _debugStream << "NULL entity\n";
			}
			else {
				newSet->add(currMention->getUID(), guessedEntity->getID());
				_debugStream << "LINKED to entity " << guessedEntity->getID() << "\n";
			}
			results[nResults++] = newSet;

		}
		//otherwise we ignore this pronoun

	}
	return nResults;
}

int SpanishPMPronounLinker::getLinkGuesses(EntitySet *currSolution,
								  Mention *currMention,
								  LinkGuess results[],
								  int max_results)
{
	int nResults = 0;
	const int MAX_CANDIDATES = 50;
	HobbsDistance::SearchResult candidates[MAX_CANDIDATES];
	int nCandidates;
	const SynNode *pronNode = currMention->node;

	while (pronNode->getParent() != NULL && !pronNode->getParent()->getSingleWord().is_null())
		pronNode = pronNode->getParent();

	Symbol headWord, parWord;
	headWord = pronNode->getHeadWord();
	parWord = PronounLinkerUtils::getAugmentedParentHeadWord(pronNode);
	// antTNG is the (type, number, gender) tuple of the antecedent.
	Symbol antTNG[3];
	double prior_ant_prob, hobbs_dist_prob, pronoun_head_prob, pronoun_parword_prob;
	LinkGuess thisGuess;

	// first, retrieve hobbs candidates
	nCandidates = HobbsDistance::getCandidates(pronNode, _previousParses, candidates, MAX_CANDIDATES);

	_debugStream << "\nPRONOUN: " << pronNode->getHeadWord().to_string() << "  " << pronNode->toFlatString() << ". " << nCandidates << " candidates found.\n";
	
	Symbol nullSym = SymbolConstants::nullSymbol;
	antTNG[0] = nullSym;
	antTNG[1] = nullSym;
	antTNG[2] = nullSym;

	Symbol nullToAnt[2] = {nullSym, PronounLinkerUtils::combineSymbols(antTNG, 3)};
	prior_ant_prob = _priorAntModel->getProbability(nullToAnt);

	Symbol nullToDist[2] = {nullSym, nullSym};	//no distance available if no antecedent
	hobbs_dist_prob = _hobbsDistanceModel->getProbability(nullToDist);

	Symbol antTNGToPronoun[4] = {nullSym, nullSym, nullSym, headWord};
	pronoun_head_prob = _pronounGivenAntModel->getProbability(antTNGToPronoun);

	Symbol antTypeToParWord[2] = {PronounLinkerUtils::augmentIfPOS(nullSym, pronNode), parWord};
	pronoun_parword_prob = _parWordGivenAntModel->getProbability(antTypeToParWord);

	thisGuess.guess = NULL;
	thisGuess.score = prior_ant_prob + hobbs_dist_prob + pronoun_head_prob + pronoun_parword_prob;
	thisGuess.sentence_num = LinkGuess::NO_SENTENCE;

	sprintf(thisGuess.debug_string, "CAND: %s [%s,%s,%s,%s] sum=%g [%g %g %g %g]",
			"NULL", nullToAnt[1].to_debug_string(), nullToDist[1].to_debug_string(),
			antTNGToPronoun[3].to_debug_string(), antTypeToParWord[1].to_debug_string(),
			exp(thisGuess.score), exp(prior_ant_prob), exp(hobbs_dist_prob),
			exp(pronoun_head_prob), exp(pronoun_parword_prob));

	results[0] = thisGuess;
	nResults++;

	/*
	_debugStream << "NULL candidate: " << debugTerminals(thisGuess.guess) << "\"";
	_debugStream << "\n\tSCORE:  P(" << nullToAnt[1].to_debug_string() << ") = " << exp(prior_ant_prob)
			<< "\tP(" << debugTerminals(pronNode) << "|TNG) = " << exp (pronoun_head_prob) << "\n\t        P(" << nullToDist[1].to_debug_string()
			<< ") = " << exp(hobbs_dist_prob) << "\t\t\t   P(" << antTypeToParWord[1].to_debug_string() << "|"
			<< antTypeToParWord[0].to_debug_string() << ") = " << exp(pronoun_parword_prob) << "\n";
	*/
		
	//now evaluate each of the hobbs candidates
	int i;
	for (i = 0; i < nCandidates; i++) {
		//prior probability of antecedent
		//determine type, number, gender
		//get type
		const SynNode *guessedNode = candidates[i].node;

		Mention *thisMention = currSolution->getMentionSet(candidates[i].sentence_number)
			->getMentionByNode(guessedNode);
		Entity *thisEntity = NULL;
		if(thisMention != NULL) thisEntity = currSolution->getEntityByMention(thisMention->getUID());
		
		bool linked_badly = false;
		if (thisEntity != NULL &&
			_isLinkedBadly(currSolution, currMention, thisEntity))
			linked_badly = true;

		if (_isSpeakerEntity(thisEntity, currSolution))
			continue;

		if(thisEntity == NULL)
			antTNG[0] = Guesser::guessType(guessedNode, thisMention);
		else antTNG[0] = thisEntity->getType().getName();
		//DONE getting type
		antTNG[1] = Guesser::guessNumber(guessedNode, thisMention);
		antTNG[2] = Guesser::guessGender(guessedNode, thisMention);
		nullToAnt[1] = PronounLinkerUtils::combineSymbols(antTNG, 3);
		prior_ant_prob = _priorAntModel->getProbability(nullToAnt);

		//hobbs distance probability
		nullToDist[1] = PronounLinkerUtils::getNormDistSymbol(i+1);
		hobbs_dist_prob = _hobbsDistanceModel->getProbability(nullToDist);
		//pronoun headword probability
		antTNGToPronoun[0] = antTNG[0];
		antTNGToPronoun[1] = antTNG[1];
		antTNGToPronoun[2] = antTNG[2];
		pronoun_head_prob = _pronounGivenAntModel->getProbability(antTNGToPronoun);
		//pronoun parword probability
		antTypeToParWord[0] = PronounLinkerUtils::augmentIfPOS(antTNG[0], pronNode);
		pronoun_parword_prob = _parWordGivenAntModel->getProbability(antTypeToParWord);

		thisGuess.guess = guessedNode;
		thisGuess.score = prior_ant_prob + hobbs_dist_prob + pronoun_head_prob + pronoun_parword_prob;
		thisGuess.sentence_num = candidates[i].sentence_number;

		sprintf(thisGuess.debug_string, "CAND: %s [%s,%s,%s,%s] sum=%g [%g %g %g %g]",
			debugTerminals(thisGuess.guess), nullToAnt[1].to_debug_string(), nullToDist[1].to_debug_string(),
			antTNGToPronoun[3].to_debug_string(), antTypeToParWord[1].to_debug_string(),
			exp(thisGuess.score), exp(prior_ant_prob), exp(hobbs_dist_prob),
			exp(pronoun_head_prob), exp(pronoun_parword_prob));

		/*
		_debugStream << "CHOICE: [S" << thisGuess.sentence_num << ", M" <<
			(thisMention == NULL ? -1 : thisMention->getUID()) << ", E" <<
			(thisEntity == NULL ? -1 : thisEntity->getID()) << "] \"" << debugTerminals(guessedNode) << "\"";
		_debugStream << "\n\tSCORE:  P(" << nullToAnt[1].to_debug_string() << ") = " << exp(prior_ant_prob)
			<< "\tP(" << debugTerminals(pronNode) << "|TNG) = " << exp (pronoun_head_prob) << "\n\t        P(" << nullToDist[1].to_debug_string()
			<< ") = " << exp(hobbs_dist_prob) << "\t\t\t   P(" << antTypeToParWord[1].to_debug_string() << "|"
			<< antTypeToParWord[0].to_debug_string() << ") = " << exp(pronoun_parword_prob) << "\n";
		*/

		//now update the sorted list of candidates
		int j;
		for (j = 0; j < nResults; j++) {
			if (thisGuess.score > results[j].score) {
				if (nResults == max_results) nResults--;
				for (int k = nResults; k > j; k--)
					results[k] = results[k-1];
				results[j] = thisGuess;
				nResults++;
				break;
			}
		}
		if (j == nResults && nResults < max_results) {
			results[nResults] = thisGuess;
			nResults++;
		}
	}
	//DEBUG
	for (int k = 0; k < nResults; k++) {
		_debugStream << results[k].debug_string << "\n";
	}

	_debugStream << "CHOSE : [S" << results[0].sentence_num << " " << debugTerminals(results[0].guess) << "]\n";
	_debugStream << "SEARCH: " << headWord.to_debug_string() << "-" << debugTerminals(results[0].guess) << "\n";
	return nResults;
}


bool SpanishPMPronounLinker::_isLinkedBadly(EntitySet *currSolution, Mention* pronMention,
								     Entity* linkedEntity)
{
	const SynNode *pronNode = pronMention->getNode();

	// "it" shouldn't be linked to person
	/*
	if ((pronNode->getHeadWord() == SpanishWordConstants::IT ||
		 pronNode->getHeadWord() == SpanishWordConstants::ITS) &&
		 linkedEntity->getType() == EntityType::getPERType()) 
	{	
		return true;
	}
	*/

	// if a pronoun is linked to an entity it modifies, then it is badly linked
	const SynNode *parentNode = pronNode->getParent();
	if (!parentNode) return false;

	for (int j = 0; j < parentNode->getNChildren(); j++) {
		if (j == parentNode->getHeadIndex()) return false;
		if (parentNode->getChild(j) == pronNode) break;
	}

	const MentionSet *ms = currSolution->getMentionSet(pronMention->getSentenceNumber());
	const Mention *parentMention = ms->getMentionByNode(parentNode);

	if (!parentMention) return false;
	Entity *parentEntity = currSolution->getEntityByMention(parentMention->getUID());
	return parentEntity == linkedEntity;

}

char *SpanishPMPronounLinker::debugTerminals(const SynNode *node) {
	static Symbol terminals[30];
	static char terminalString[512];
	if (node == NULL) {
		strcpy(terminalString, "NULL");
		return terminalString;
	}
	int nTerminals = node->getTerminalSymbols(terminals, 30);
	strcpy(terminalString, "");
	for (int i = 0; i < nTerminals; i++) {
		strcat(terminalString, terminals[i].to_debug_string());
		if(i != nTerminals - 1) 
			strcat(terminalString, " ");
	}
	return terminalString;
}

// given a param file of files, read them in and initialize models.
// this may (should) change when the param thingy is implemented
void SpanishPMPronounLinker::_initialize(const char* model_prefix)
{
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	std::string model_prefix_str(model_prefix);

	std::string buffer = model_prefix_str + ".antprior";
	verifyEntityTypes(buffer.c_str());
	instream.open(buffer.c_str());
	_priorAntModel = _new ProbModel(2, instream, false);

	buffer = model_prefix_str + ".distmodel";
	instream.open(buffer.c_str());
	_hobbsDistanceModel = _new ProbModel(2, instream, false);

	buffer = model_prefix_str + ".pronheadmodel";
	instream.open(buffer.c_str());
	_pronounGivenAntModel = _new ProbModel(4, instream, false);

	buffer = model_prefix_str + ".parwordmodel";
	instream.open(buffer.c_str());
	_parWordGivenAntModel = _new ProbModel(2, instream, false);

	instream.close();
}

void SpanishPMPronounLinker::verifyEntityTypes(const char *file_name) {
	boost::scoped_ptr<UTF8InputStream> priorAntStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorAntStream(*priorAntStream_scoped_ptr);
	priorAntStream.open(file_name);

	if (priorAntStream.fail()) {
		throw UnexpectedInputException(
			"SpanishPronounLinker::verifyEntityTypes()",
			"Unable to open pronoun linker model files.");
	}

	int n_entries;
	priorAntStream >> n_entries;

	for (int i = 0; i < n_entries; i++) {
		UTF8Token token;
		priorAntStream >> token; // (
		priorAntStream >> token; // (
		priorAntStream >> token; // :NULL

		priorAntStream >> token; // enitity type plus other garbage
		wchar_t etype_str[100];
		wcsncpy(etype_str, token.chars(), 99);
		wchar_t *dot = wcschr(etype_str, L'.');
		*dot = L'\0';
		Symbol etype(etype_str);
		try {
			if (wcscmp(etype_str, L":NULL"))
				EntityType(Symbol(etype_str));
		}
		catch (UnexpectedInputException &) {
			std::string message = "";
			message += "The pronoun linker model refers to "
					   "at least one unrecognized entity type: ";
			message += etype.to_debug_string();
			throw UnexpectedInputException(
				"SpanishPronounLinker::verifyEntityTypes()",
				message.c_str());
		}

		priorAntStream >> token; // )
		priorAntStream >> token; // number
		priorAntStream >> token; // number
		priorAntStream >> token; // number
		priorAntStream >> token; // )
	}

	priorAntStream.close();
}

bool SpanishPMPronounLinker::_isSpeakerMention(Mention *ment) { 
		return (_docTheory->isSpeakerSentence(ment->getSentenceNumber()));
}
	
bool SpanishPMPronounLinker::_isSpeakerEntity(Entity *ent, EntitySet *ents) {
	if (ent == 0)
		return false;
	for (int i=0; i < ent->mentions.length(); i++) {
		Mention* ment = ents->getMention(ent->mentions[i]);
		if (_isSpeakerMention(ment))
			return true;
	}
	return false;
}

