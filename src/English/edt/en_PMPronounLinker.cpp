// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/en_PMPronounLinker.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "English/parse/en_NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "English/common/en_WordConstants.h"
#include "English/parse/en_STags.h"
#include "Generic/edt/PronounLinkerUtils.h"
#include "Generic/edt/Guesser.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/EntityType.h"
#include "Generic/edt/discmodel/CorefUtils.h"

#include <string.h>
#include <math.h>
#include <boost/scoped_ptr.hpp>

DebugStream EnglishPMPronounLinker::_debugOut;

EnglishPMPronounLinker::EnglishPMPronounLinker()
	: _priorAntModel(0), _hobbsDistanceModel(0), _pronounGivenAntModel(0),
	  _parWordGivenAntModel(0)
{
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	//initialize 4 ProbModels
	//also initialize guesser
	
	std::string model_prefix = ParamReader::getRequiredParam("pronlink_model");
	_initialize(model_prefix.c_str());

	Guesser::initialize();
	HobbsDistance::initialize();

	_debugOut.init(Symbol(L"pronlink_debug_file"), false);
}

EnglishPMPronounLinker::~EnglishPMPronounLinker() {
	Guesser::destroy();
	delete _priorAntModel;
	delete _hobbsDistanceModel;
	delete _pronounGivenAntModel;
	delete _parWordGivenAntModel;
}

void EnglishPMPronounLinker::addPreviousParse(const Parse *parse) {
	_previousParses.push_back(parse);
}

void EnglishPMPronounLinker::resetPreviousParses() {
	_previousParses.clear();
}

int EnglishPMPronounLinker::linkMention (LexEntitySet * currSolution,
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

	max_results = max_results>64 ? 64 : max_results;

	// we shouldn't try to resolve "here" and "there"
	if (pronounNode->getHeadWord() == EnglishWordConstants::HERE ||
		pronounNode->getHeadWord() == EnglishWordConstants::THERE)
	{
		results[0] = currSolution->fork();

		// EMB 10/18/05: 
		// do we ever want to actually type these things? It seems to always lower scores, sadly
		/*if (pronounNode->getHeadWord() == EnglishWordConstants::HERE) {
			const SynNode *nextTerminal = pronounNode->getNextTerminal();
			if (nextTerminal &&
				(nextTerminal->getHeadWord() == EnglishWordConstants::IS ||
				nextTerminal->getHeadWord() == EnglishWordConstants::_S ||
				nextTerminal->getHeadWord() == EnglishWordConstants::ARE))
			{
				// no mention for "here is" or "here are" or "here's"
			} else {
				results[0]->getMention(currMention->getUID())->setEntityType(EntityType::getLOCType();				
				results[0]->addNew(currMention->getUID(), EntityType::getLOCType());
			}
		}*/

		return 1;
	}

	if (_use_correct_answers) {
		// if we are using correct mentions, we only want to link pronouns with either
		// a recognized type (totally unfair) or, 
		// as in the EDR conditional eval, with type UNDET.
		if (currMention->getEntityType() == EntityType::getOtherType()) {
			results[0] = currSolution->fork();
			return 1;
		}
	}

	int nGuesses = 0;
	if (EnglishNodeInfo::isWHQNode(pronounNode)) {
		nGuesses = getWHQLinkGuesses(currSolution, currMention, linkGuesses, max_results);
	} else {
//		nGuesses = getLinkGuesses(currSolution, currMention, linkGuesses, max_results);
		nGuesses = getLinkGuesses(currSolution, currMention, linkGuesses, 64);
		if (nGuesses > max_results) nGuesses = max_results;
	}

	int nResults = 0;
	bool alreadyProcessedNullDecision = false;

	// make sure at least one valid decision exists, otherwise add one
	// this can happen because we drop the ones that are using simple coref instead
	bool found_valid_guess = false;
	for (int i = 0; i < nGuesses; i++) {
		const SynNode *guessedNode = linkGuesses[i].guess;
		if (guessedNode == NULL ||
			!currSolution->getMentionSet(linkGuesses[i].sentence_num)->getMentionByNode(guessedNode) || 
			!currSolution->getMentionSet(linkGuesses[i].sentence_num)->getMentionByNode(guessedNode)->getEntityType().useSimpleCoref())
		{
			found_valid_guess = true;
			break;
		}
	}
	if (!found_valid_guess) {
		linkGuesses[0].guess = NULL;
		linkGuesses[0].score = 1.0f;
		linkGuesses[0].sentence_num = LinkGuess::NO_SENTENCE;
		linkGuesses[0].linkConfidence = 1.0;
		nGuesses = 1;
	}

	for (int i = 0; i < nGuesses; i++) {
		const SynNode *guessedNode = linkGuesses[i].guess;
		
		//determine mention from node
		Mention *guessedMention = (guessedNode != NULL) ? 
			currSolution->getMentionSet(linkGuesses[i].sentence_num)->getMentionByNode(guessedNode) : NULL;

		//find its parent mention iff the mention type is NONE
		if (guessedMention != NULL && guessedMention->getMentionType() == Mention::NONE) {
			const SynNode *tmpNode = guessedNode;
			while (guessedMention->getMentionType() == Mention::NONE && tmpNode->getParent() != NULL && 
				tmpNode->getParent()->hasMention() && tmpNode->getParent()->getHead() == tmpNode) 
			{
				tmpNode = tmpNode->getParent();
				guessedMention = currSolution->getMentionSet(linkGuesses[i].sentence_num)->getMentionByNode(tmpNode);
			}
		}

		//determine entity from mention
		Entity *guessedEntity = (guessedMention != NULL) ?
			currSolution->getEntityByMention(guessedMention->getUID()) : NULL;

		// Don't link to "simple coref" entities; too risky
		if (guessedEntity != NULL && guessedEntity->getType().useSimpleCoref()) {
			continue;
		}	

		//many different pronoun resolution decisions can lead to a "no-link" result.
		//therefore, only fork the LexEntitySet for the top scoring no-link guess.
		if (guessedEntity != NULL || !alreadyProcessedNullDecision) {
			LexEntitySet *newSet = currSolution->fork();
			Mention *newMention = newSet->getMention(currMention->getUID());
			newMention->setLinkConfidence(linkGuesses[i].linkConfidence);

			if (guessedMention != NULL) {
				_debugOut << "chose mention " << guessedMention->getUID() << " type " << guessedMention->getMentionType() << "\n";
				// update the entity type iff we're linking to an entity
				if (guessedEntity != NULL)
					newMention->setEntityType(guessedMention->getEntityType());
			} else {
				_debugOut << "NULL mention\n";
			}

			if (guessedEntity == NULL) {
				alreadyProcessedNullDecision = true;
				Symbol headword = currMention->getNode()->getHeadWord();
				// for pronouns we gave types to in relation finding, say
				if (currMention->getEntityType().isRecognized()) {			
					newMention->setEntityType(currMention->getEntityType());				
					newSet->addNew(currMention->getUID(), currMention->getEntityType());
					_debugOut << "Forcing pronoun to be an entity (perhaps because it's in a relation): ";
					if (currMention->getNode()->getParent() != 0 && 
						currMention->getNode()->getParent()->getParent() != 0)
					{
						_debugOut << currMention->getNode()->getParent()->getParent()->toTextString();
					}
					_debugOut << "\n";
				} else _debugOut << "NULL entity\n";
				if (_use_correct_answers) {
					if (headword == EnglishWordConstants::HE || headword == EnglishWordConstants::HIM || 
						headword == EnglishWordConstants::HIS || headword == EnglishWordConstants::SHE || 
						headword == EnglishWordConstants::HER || headword == EnglishWordConstants::THEY ||
						headword == EnglishWordConstants::THEM || headword == EnglishWordConstants::THEIR)
						{
							newMention->setEntityType(EntityType::getPERType());
							newSet->addNew(currMention->getUID(), EntityType::getPERType());	
						} else if (headword == EnglishWordConstants::HERE || headword == EnglishWordConstants::THERE) {
						newMention->setEntityType(EntityType::getLOCType());
						newSet->addNew(currMention->getUID(), EntityType::getLOCType());	
					} else {
						newMention->setEntityType(EntityType::getORGType());
						newSet->addNew(currMention->getUID(), EntityType::getORGType());	
					}
				}
			}
			else {
				newSet->add(currMention->getUID(), guessedEntity->getID());
				_debugOut << "linked to entity " << guessedEntity->getID() << "\n";
			}
			results[nResults++] = newSet;

		}
		//otherwise we ignore this pronoun

	}
	return nResults;
}

int EnglishPMPronounLinker::getLinkGuesses(LexEntitySet *currSolution,
								  Mention *currMention,
								  LinkGuess results[],
								  int max_results)
{
	//use probability model to evaluate the likelihood of each link candidate (including "no link")
	int nResults = 0;
	Symbol nullSym = SymbolConstants::nullSymbol;
	const int MAX_CANDIDATES = 50;
	HobbsDistance::SearchResult candidates[MAX_CANDIDATES];
	int nCandidates;	
	double prior_ant_prob, hobbs_dist_prob, pronoun_head_prob, pronoun_parword_prob;
	LinkGuess thisGuess;

	// antTNG is the (type, number, gender) tuple of the antecedent.
	Symbol antTNG[3];
	antTNG[0] = nullSym;
	antTNG[1] = nullSym;
	antTNG[2] = nullSym;
	
	// get the pronoun node
	const SynNode *pronNode = currMention->getNode();
	while (pronNode->getParent() != NULL && !pronNode->getParent()->getSingleWord().is_null())
		pronNode = pronNode->getParent();

	Symbol headWord = pronNode->getHeadWord();
	Symbol parWord = PronounLinkerUtils::getAugmentedParentHeadWord(pronNode);
	
	// retrieve hobbs antecedent candidates
	nCandidates = HobbsDistance::getCandidates(pronNode, _previousParses, candidates, MAX_CANDIDATES);

	if (_debugOut.isActive())
		_debugOut << "\nPRONOUN: " << headWord.to_debug_string() << "  " << pronNode->toFlatString() << ". " << nCandidates << " candidates found.\n";

	//now, evaluate the possibility of no link
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

	_snprintf(thisGuess.debug_string, sizeof(thisGuess.debug_string), "CAND: %s [%s,%s,%s,%s] sum=%g [%g %g %g %g]",
			"NULL", nullToAnt[1].to_debug_string(), nullToDist[1].to_debug_string(),
			antTNGToPronoun[3].to_debug_string(), antTypeToParWord[1].to_debug_string(),
			exp(thisGuess.score), exp(prior_ant_prob), exp(hobbs_dist_prob),
			exp(pronoun_head_prob), exp(pronoun_parword_prob));

	results[0] = thisGuess;
	nResults++;

	_debugOut << "NULL candidate: " <<
		//debugScoreString(prior_ant_prob, nullToAnt, hobbs_dist_prob, nullToDist,
		//pronoun_head_prob, antTNGToPronoun, pronoun_parword_prob, antTypeToParWord) <<
		"\n";
	
	//now evaluate each of the hobbs candidates
	for (int i = 0; i < nCandidates; i++) {
		const SynNode *guessedNode = candidates[i].node;

		if (guessedNode == NULL) {
			throw InternalInconsistencyException("EnglishPMPronounLinker::getLinkGuesses()",
									"Attempt to evaluate link to null SynNode.");
		}

		//determine mention from node
		Mention *thisMention = currSolution->getMentionSet(candidates[i].sentence_number)->getMentionByNode(guessedNode);
		
		//find its parent mention iff the mention type is NONE
		if (thisMention != NULL && thisMention->getMentionType() == Mention::NONE) {
			const SynNode *tmpNode = guessedNode;
			while (thisMention->getMentionType() == Mention::NONE && tmpNode->getParent() != NULL && 
				tmpNode->getParent()->hasMention() && tmpNode->getParent()->getHead() == tmpNode) 
			{
				tmpNode = tmpNode->getParent();
				thisMention = currSolution->getMentionSet(candidates[i].sentence_number)->getMentionByNode(tmpNode);
			}
		}

		//determine entity by mention
		Entity *thisEntity = (thisMention != NULL) ? currSolution->getEntityByMention(thisMention->getUID()) : NULL;  

		bool linked_badly = false;
		if (thisEntity != NULL &&_isLinkedBadly(currSolution, currMention, thisEntity))
			linked_badly = true;

		if (_isSpeakerEntity(thisEntity, currSolution))
			continue;

		if (_use_correct_answers) {
			// this is probably safe for non-ca mode, too, but let's be cautious
			if (thisEntity != NULL &&
				!thisEntity->getType().matchesPER() &&
				(headWord == EnglishWordConstants::HE || headWord == EnglishWordConstants::HIM || 
				 headWord == EnglishWordConstants::HIS || headWord == EnglishWordConstants::SHE || 
				 headWord == EnglishWordConstants::HER))
				{
					continue;
				}
		}

		// prior probability of antecedent
		antTNG[0] = (thisEntity == NULL) ? Guesser::guessType(guessedNode, thisMention) : thisEntity->getType().getName();
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

		_snprintf(thisGuess.debug_string, sizeof(thisGuess.debug_string), "CAND: %s [%s,%s,%s,%s] sum=%g [%g %g %g %g]",
			debugTerminals(thisGuess.guess), nullToAnt[1].to_debug_string(), nullToDist[1].to_debug_string(),
			antTNGToPronoun[3].to_debug_string(), antTypeToParWord[1].to_debug_string(),
			exp(thisGuess.score), exp(prior_ant_prob), exp(hobbs_dist_prob),
			exp(pronoun_head_prob), exp(pronoun_parword_prob));
		/*
		_debugOut << "CHOICE: [S" << thisGuess.sentence_num << ", M" <<
			(thisMention == NULL ? -1 : thisMention->ID) << ", E" <<
			(thisEntity == NULL ? -1 : thisEntity->getID()) << "] \"" << debugTerminals(guessedNode) << "\"";
		_debugOut << "\n\tSCORE:  P(" << nullToAnt[1].to_debug_string() << ") = " << exp(prior_ant_prob)
			<< "\tP(" << debugTerminals(pronNode) << "|TNG) = " << exp (pronoun_head_prob) << "\n\t        P(" << nullToDist[1].to_debug_string()
			<< ") = " << exp(hobbs_dist_prob) << "\t\t\t   P(" << antTypeToParWord[1].to_debug_string() << "|"
			<< antTypeToParWord[0].to_debug_string() << ") = " << exp(pronoun_parword_prob) << "\n";
		*/
//		debugScoreString(prior_ant_prob, nullToAnt, hobbs_dist_prob, nullToDist,
//		pronoun_head_prob, antTNGToPronoun, pronoun_parword_prob, antTypeToParWord) <<

		//now update the sorted list of candidates
		int j;
		for (j = 0; j < nResults; j++) {
			if (thisGuess.score > results[j].score && !linked_badly) {
				if (nResults == max_results) nResults--;
				for (int k = nResults; k > j; k--)
					results[k] = results[k-1];
				results[j] = thisGuess;
				nResults++;
				//_debugOut << "replaced " << j << " with " << thisGuess.score << ".\n";
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
		_debugOut << results[k].debug_string << "\n";
		//_debugOut << results[k].score << "\n";
	}
	_debugOut << "CHOSE : [S" << results[0].sentence_num << " " << debugTerminals(results[0].guess) << "]\n";
	_debugOut << "SEARCH: " << headWord.to_debug_string() << "-" << debugTerminals(results[0].guess) << "\n";

	// compute confidences for the linking options
	CorefUtils::computeConfidence(results, nResults);

	return nResults;
}


bool EnglishPMPronounLinker::_isLinkedBadly(LexEntitySet *currSolution, Mention* pronMention, Entity* linkedEntity) {
	const SynNode *pronNode = pronMention->getNode();

	
	// "it" shouldn't be linked to person
	if ((pronNode->getHeadWord() == EnglishWordConstants::IT ||
		 pronNode->getHeadWord() == EnglishWordConstants::ITS) &&
		 linkedEntity->getType() == EntityType::getPERType()) 
	{	
		return true;
	}

	// if a pronoun is linked to an entity it modifies, then it is badly linked
	const SynNode *parentNode = 0;
	const SynNode *currentNode = pronNode;

	while ((parentNode = currentNode->getParent()) != 0) {
		for (int j = 0; j < parentNode->getNChildren(); j++) {
			
			// if currentNode is not a premod, continue to next parent
			if (j > parentNode->getHeadIndex() ||
				(j == parentNode->getHeadIndex() && currentNode == pronNode))
				break;

			if (parentNode->getChild(j) == currentNode) {
				// currentNode is a premod, check to see if parent is in the same entity as \
				// linkedEntity
				const MentionSet *ms = currSolution->getMentionSet(pronMention->getSentenceNumber());
				const Mention *parentMention = ms->getMentionByNode(parentNode);

				if (parentMention) {
					Entity *parentEntity = currSolution->getEntityByMention(parentMention->getUID());
					if (parentEntity == linkedEntity) return true;
				}
			}
		}
		currentNode = parentNode;
	}

	return false;
}

int EnglishPMPronounLinker::getWHQLinkGuesses(LexEntitySet *currSolution,
								  Mention *currMention,
								  LinkGuess results[],
								  int max_results)
{
	results[0].guess = NULL;
	results[0].score = 1;
	results[0].sentence_num = LinkGuess::NO_SENTENCE;
	results[0].linkConfidence = 1.0;
	_snprintf(results[0].debug_string, sizeof(results[0].debug_string), "NULL CAND");

	const SynNode *pronNode = currMention->node;
	const SynNode *parent = pronNode->getParent();
	if (parent == 0)
		return 1;

	if (pronNode->getTag() == EnglishSTags::WHNP || pronNode->getTag() == EnglishSTags::WHADVP) {
		if (parent->getTag() == EnglishSTags::SBAR) {
			parent = parent->getParent();
			if (parent == 0) return 1;
			if (parent->getTag() == EnglishSTags::SBAR) {
				parent = parent->getParent();
				if (parent == 0) return 1;
			}
			if (parent->getTag() == EnglishSTags::NP) {
				results[0].guess = parent;
				results[0].score = 1;
				results[0].sentence_num = currMention->getSentenceNumber();
				//sprintf(results[0].debug_string, "NP parent of WHNP: %s",
				//	parent->toDebugTextString());
				return 1;
			} else return 1;
		} else return 1;
	}

	if (pronNode->getTag() == EnglishSTags::WPDOLLAR) {
		if (parent->getTag() == EnglishSTags::WHNP) {
			parent = parent->getParent();
			if (parent == 0) return 1;
			if (parent->getTag() == EnglishSTags::SBAR) {
				parent = parent->getParent();
				if (parent == 0) return 1;
				if (parent->getTag() == EnglishSTags::SBAR) {
					parent = parent->getParent();
					if (parent == 0) return 1;
				}
				if (parent->getTag() == EnglishSTags::NP) {
					results[0].guess = parent;
					results[0].score = 1;
					results[0].sentence_num = currMention->getSentenceNumber();
					//sprintf(results[0].debug_string, "NP parent of WHNP/WP$: %s",
					//	parent->toDebugTextString());
				} else return 1;
			} else return 1;
		} else return 1;
	}

	return 1;
}

char *EnglishPMPronounLinker::debugTerminals(const SynNode *node) {
	static const size_t debug_string_length = 1000;
	static Symbol terminals[30];
	static char terminalString[debug_string_length];
	if(node == NULL) {
		strcpy(terminalString, "NULL");
		return terminalString;
	}
	int nTerminals = node->getTerminalSymbols(terminals, 30);
	strcpy(terminalString, "");
	for(int i=0; i<nTerminals; i++) {
		if (!(strlen(terminalString) + strlen(terminals[i].to_debug_string()) + 1 >
			  debug_string_length))
		{
			strcat(terminalString, terminals[i].to_debug_string());
			if(i!=nTerminals-1) strcat(terminalString, " ");
		}
	}
	return terminalString;
}

// given a param file of files, read them in and initialize models.
// this may (should) change when the param thingy is implemented
void EnglishPMPronounLinker::_initialize(const char* model_prefix)
{
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	char buffer[501];

	_snprintf(buffer,sizeof(buffer),"%s.antprior", model_prefix);
	verifyEntityTypes(buffer);
	instream.open(buffer);
	_priorAntModel = _new ProbModel(2, instream, false);

	_snprintf(buffer,sizeof(buffer),"%s.distmodel", model_prefix);
	instream.open(buffer);
	_hobbsDistanceModel = _new ProbModel(2, instream, false);

	_snprintf(buffer,sizeof(buffer),"%s.pronheadmodel", model_prefix);
	instream.open(buffer);
	_pronounGivenAntModel = _new ProbModel(4, instream, false);

	_snprintf(buffer,sizeof(buffer),"%s.parwordmodel", model_prefix);
	instream.open(buffer);
	_parWordGivenAntModel = _new ProbModel(2, instream, false);

	instream.close();
}


void EnglishPMPronounLinker::verifyEntityTypes(char *file_name) {
	boost::scoped_ptr<UTF8InputStream> priorAntStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorAntStream(*priorAntStream_scoped_ptr);
	priorAntStream.open(file_name);

	if (priorAntStream.fail()) {
		throw UnexpectedInputException(
			"EnglishPMPronounLinker::verifyEntityTypes()",
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
				"EnglishPMPronounLinker::verifyEntityTypes()",
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


bool EnglishPMPronounLinker::_isSpeakerMention(Mention *ment) { 
		return (_docTheory->isSpeakerSentence(ment->getSentenceNumber()));
	}
	
bool EnglishPMPronounLinker::_isSpeakerEntity(Entity *ent, LexEntitySet *ents) {
	if (ent == 0)
		return false;
	for (int i=0; i < ent->mentions.length(); i++) {
		Mention* ment = ents->getMention(ent->mentions[i]);
		if (_isSpeakerMention(ment))
			return true;
	}
	return false;
}

