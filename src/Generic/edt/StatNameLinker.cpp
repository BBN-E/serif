// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/StatNameLinker.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/EntityGuess.h"
//note: the following header gives language specific functionality
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"

#include <math.h>
#include <stdio.h>
#include <boost/scoped_ptr.hpp>

DebugStream &StatNameLinker::_debugOut = DebugStream::referenceResolverStream;

const double StatNameLinker::LOG_OF_ZERO = -10000;
StatNameLinker::StatNameLinker()
	: _genericModel(0), _newOldModel(0), _filter_by_entity_subtype(false)
{
	_generic_unseen_weights = _new double[EntityType::getNTypes()];
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	
	std::string filename = ParamReader::getRequiredParam("generic_model_file");
	instream.open(filename.c_str());
	_genericModel = _new ProbModel(2, instream, false);

	filename = ParamReader::getRequiredParam("namelink_newold_model_file");
	instream.open(filename.c_str());
	_newOldModel = _new ProbModel(2, instream, false);


	useRules = ParamReader::getRequiredTrueFalseParam("use_simple_rule_namelink");
	if (useRules)
		_simpleRuleNameLinker = _new SimpleRuleNameLinker();


	loadWeights();

	_symOld = Symbol(L"OLD");
	_symNew = Symbol(L"NEW");

	_symNumbers[0] = Symbol(L"0");
	_symNumbers[1] = Symbol(L"1");
	_symNumbers[2] = Symbol(L"2");
	_symNumbers[3] = Symbol(L"3");
	_symNumbers[4] = Symbol(L"4");

	instream.close();

	// FILTERING BY ENTITY SUBTYPE
	_filter_by_entity_subtype = ParamReader::isParamTrue("do_coref_entity_subtype_filtering");
}

StatNameLinker::~StatNameLinker() {
	delete [] _generic_unseen_weights;
	AbbrevTable::destroy();
	NameLinkFunctions::destroyDataStructures();
	delete _genericModel;
	delete _newOldModel;
	if (useRules)
		delete _simpleRuleNameLinker;
}

//load generic and specific unseen weights
void StatNameLinker::loadWeights() {
	// initialize the arrays (to something other than 0)
	int i;
	for (i=0; i < EntityType::getNTypes(); i++) {
		LexEntity::unseen_weights[i] = 0.1;
		_generic_unseen_weights[i] = 0.1;
	}

	// notice the specific weights actually reside in LexEntity!
	//The generic weights are local.
	std::string spec_file = ParamReader::getRequiredParam("specific_unseen_weights");
	boost::scoped_ptr<UTF8InputStream> specStream_scoped_ptr(UTF8InputStream::build(spec_file.c_str()));
	UTF8InputStream& specStream(*specStream_scoped_ptr);
	int n_entries = -1;
	specStream >> n_entries;
	if (specStream.eof() || n_entries < 0) 
	{
		std::stringstream errMsg;
		errMsg << "Could not read file:\n";
		errMsg << spec_file;
		errMsg << "\nSpecified by parameter 'specific_unseen_weights'";
		throw UnrecoverableException("StatNameLinker::loadWeights()", errMsg.str().c_str());
	}
	UTF8Token token;
	if (n_entries != EntityType::getNTypes()) {
		SessionLogger::dbg("spec_wts_0") << "About to read in " << n_entries
			<< " specific weights; there have been " << EntityType::getNTypes()
			<< " types read in.";
	}


	for (i=0; i < n_entries; i++) {
		// Expect paren, type, val, paren
		specStream >> token;
		if (token.symValue() != SymbolConstants::leftParen) {
			SessionLogger::err("read_ch_0") << "Expected '('; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
		specStream >> token;
		int pos = -1;
		try {
			pos = EntityType(token.symValue()).getNumber();
		}
		catch (UnexpectedInputException &) {
			std::string message = "";
			message += "The name linker model refers to "
					   "at least one unrecognized entity type: ";
			message += token.symValue().to_debug_string();
			throw UnexpectedInputException("StatNameLinker::loadWeights()",
										   message.c_str());
		}
		double val;
		specStream >> val;
		if (pos >=0)
			LexEntity::unseen_weights[pos] = val;

		specStream >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			SessionLogger::err("read_ch_1") << "Expected ')'; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
	}
	specStream.close();

	std::string gen_file = ParamReader::getRequiredParam("generic_unseen_weights");
	boost::scoped_ptr<UTF8InputStream> genStream_scoped_ptr(UTF8InputStream::build(gen_file.c_str()));
	UTF8InputStream& genStream(*genStream_scoped_ptr);
	n_entries = -1;
	genStream >> n_entries;
	if (genStream.eof() || n_entries < 0) {
		std::stringstream errMsg;
		errMsg << "Could not read file:\n";
		errMsg << gen_file;
		errMsg << "\nSpecified by parameter generic_unseen_weights";
		throw UnrecoverableException("StatNameLinker::loadWeights()", errMsg.str().c_str());
	}
	// JCS - 2/23/04 don't really need the generic weight for OTH - it doesn't ever get used.
	if (n_entries != EntityType::getNTypes() && n_entries != EntityType::getNTypes() - 1) {
		SessionLogger::dbg("gen_wts_0") << "About to read in " << n_entries
			<< " generic weights; there have been " << EntityType::getNTypes()
			<< " types read in.";
	}

	for (i=0; i < n_entries; i++) {
		// Expect paren, type, val, paren
		genStream >> token;
		if (token.symValue() != SymbolConstants::leftParen) {
			SessionLogger::err("exp_ch_5") << "Expected '('; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
		genStream >> token;
		int pos = -1;
		try {
			pos = EntityType(token.symValue()).getNumber();
		}
		catch (UnexpectedInputException &) {
			std::string message = "";
			message += "The name linker model refers to "
					   "at least one unrecognized entity type: ";
			message += token.symValue().to_debug_string();
			throw UnexpectedInputException("StatNameLinker::loadWeights()",
										   message.c_str());
		}
		double val;
		genStream >> val;
		if (pos >= 0)
			_generic_unseen_weights[pos] = val;

		genStream >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			SessionLogger::err("no_paren_0") << "Expected ')'; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
	}
	genStream.close();
}

int StatNameLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) {
	//retrieve the guesses for this mention, populate array with forked EntitySets, return array
	if (useRules)
		_simpleRuleNameLinker->setCurrSolution(currSolution);
	LexEntitySet * newSet = NULL;
	EntityGuess *guesses [64];
	Mention *currMention = currSolution->getMention(currMentionUID);
	int nGuesses = 0;
	_debugOut << "\nProcessing mention #" << currMention->getUID() << "\n";
	//DEBUG: print out all symbols, and next just head's symbols
	Symbol symArrayAll[16], symArrayHead[16], symArrayMentionHead[16];
//	int nAll, nHead, nMentionHead;
//	nAll = currMention->node->getTerminalSymbols(symArrayAll, 16);
//	nHead = currMention->node->getHead()->getTerminalSymbols(symArrayHead, 16);
//	nMentionHead = currMention->getHead()->getTerminalSymbols(symArrayMentionHead, 16);

	//_debugOut << "\nMention head symbols: ";
	//for (int i=0; i<nMentionHead; i++)
		//_debugOut << symArrayMentionHead[i].to_string() << " "; //Marj changed from debug_string
	//_debugOut << "\n";
	//END DEBUG
	nGuesses = guessEntity(currSolution, currMention, linkType, guesses, max_results>64 ? 64: max_results);

	for (int i = 0; i<nGuesses; i++) {
		newSet = currSolution->fork();
		EntityGuess *thisGuess = guesses[i];
		if (thisGuess->id == EntityGuess::NEW_ENTITY) {
			newSet->addNew(currMention->getUID(), thisGuess->type);
			_debugOut << "\n CREATED ENTITY #" << newSet->getNEntities()-1;
		}
		else {
			newSet->add(currMention->getUID(), thisGuess->id);
			_debugOut << "\n LINKED TO ENTITY #" << thisGuess->id;
		}
		newSet->setScore(newSet->getScore()+static_cast<float>(thisGuess->score));
		results[i] = newSet;
		delete thisGuess;
		thisGuess = guesses[i] = NULL;
		newSet->customDebugPrint(_debugOut);
	}
	_debugOut << "\nDone processing mention #" << currMention->getIndex() << "\n\n";

	return nGuesses;
}

int StatNameLinker::guessEntity(LexEntitySet * currSolution, Mention * currMention, EntityType linkType, EntityGuess *results[], int max_results) {
	_debugOut << "BEGIN_GUESSES\n";

	//this guessing algorithm relies on our already having resolved the type of the mention
	
	// Do only string-based name matching for "simple coref" types
	if (linkType.useSimpleCoref()) {
		std::wstring name_string = currMention->getAtomicHead()->toTextString();			
		const GrowableArray <Entity *> &filteredEnts = currSolution->getEntitiesByType(linkType);
		for (int i=0; i<filteredEnts.length(); i++) {
			for (int m = 0; m < filteredEnts[i]->getNMentions(); m++) {
				const Mention *ment = currSolution->getMention(filteredEnts[i]->getMention(m));
				if (ment->getMentionType() == Mention::NAME) {
					std::wstring candidate_name_string = ment->getAtomicHead()->toTextString();
					if (name_string == candidate_name_string) {
						EntityGuess * newGuess = _new EntityGuess();	
						newGuess->id = filteredEnts[i]->getID();
						newGuess->type = filteredEnts[i]->getType();
						newGuess->score = 1.0f;
						newGuess->linkConfidence = 1.0f;
						results[0] = newGuess;
						return 1;
					}
				}
			}
		}
		EntityGuess * newGuess = _new EntityGuess();	
		newGuess->id = EntityGuess::NEW_ENTITY;
		newGuess->type = linkType;
		newGuess->score = 1.0f;
		newGuess->linkConfidence = 1.0f;
		results[0] = newGuess;
		return 1;
	}

	EntityGuess * newGuess = _new EntityGuess();
	int numEntities, numEntitiesNorm;
	Symbol words[32], resolved[32], lexicalItems[100];
	int nWords, nResolved, nLexicalItems, nResults = 0;
	Symbol trans[2];

	numEntities = currSolution->getNEntities();
	numEntitiesNorm = numEntities>4 ? 4 : numEntities;

	//populate the dynamic abbreviations table if necessary
	NameLinkFunctions::populateAcronyms(currMention, linkType);

	//first score the new entity option
	_debugOut << "NEW: " << linkType.getName().to_debug_string();
	newGuess->id = EntityGuess::NEW_ENTITY;
	newGuess->type = linkType;

	trans[0] = _symNumbers[numEntitiesNorm];
	trans[1] = _symNew;
	newGuess->score = _newOldModel->getProbability(trans);
	_debugOut << " N/O: " << exp(newGuess->score);
	int i;
	nWords = currMention->getHead()->getTerminalSymbols(words, 16);
	nResolved = AbbrevTable::resolveSymbols(words, nWords, resolved, 32);
	nLexicalItems = NameLinkFunctions::getLexicalItems(resolved, nResolved, lexicalItems, 100);

	_debugOut << "\nUNRESOLVED: ";
	for (i=0; i<nWords; i++)
		_debugOut << words[i].to_string() << " "; //Marj changed from debug_string
	_debugOut << "\nRESOLVED: ";
	for (i=0; i<nResolved; i++)
		_debugOut << resolved[i].to_string() << " "; //Marj changed from debug_string
	_debugOut << "\nLEXICAL ITEMS: ";
	for (i=0; i<nLexicalItems; i++)
		_debugOut << lexicalItems[i].to_string() << " "; //Marj changed from debug_string
	_debugOut << "\n";


	for (i=0; i<nLexicalItems; i++) {
		trans[0] = newGuess->type.getName();
		trans[1] = lexicalItems[i];
		int index = EntityType(newGuess->type).getNumber();
		double lambda = _generic_unseen_weights[index];
		double thisWordScore = log (
			(1-lambda) * exp(_genericModel->getProbability(trans)) + lambda * 1.0/10000);
		newGuess->score += thisWordScore;
		_debugOut << " [" << lexicalItems[i].to_debug_string() << ", " << exp(thisWordScore) << "] ";
	}
	_debugOut << "total: " << exp (newGuess->score);
	results[0] = newGuess;
	nResults++;

	// if this is not a "linkable" type, then we do not add the link options in,
	// so a new entity must be created.
	if (linkType.isLinkable()) {
		//now score each of the possible linkings
		const GrowableArray <Entity *> &filteredEnts = currSolution->getEntitiesByType(linkType);
		for (i=0; i<filteredEnts.length(); i++) {
			if (_filter_by_entity_subtype &&
				!CorefUtils::subtypeMatch(currSolution, filteredEnts[i], currMention))
				continue;
			newGuess = _new EntityGuess();
			newGuess->id = filteredEnts[i]->getID();
			newGuess->type = filteredEnts[i]->getType();
			trans[0] = _symNumbers[numEntitiesNorm];
			trans[1] = _symOld;
			newGuess->score = _newOldModel->getProbability(trans) - log((double)filteredEnts.length());
			_debugOut << "\nOLD " << filteredEnts[i]->getID() << ": " << "N/O: " << exp(_newOldModel->getProbability(trans)) << " CHOOSE: " << filteredEnts.length();
			LexEntity *lexEntity = currSolution->getLexEntity(newGuess->id);

			double logLexicalProb = (lexEntity != NULL) ?
				lexEntity->estimate(lexicalItems, nLexicalItems) :
				LOG_OF_ZERO;

			_debugOut << " LEX: " << exp(logLexicalProb);
			newGuess->score += logLexicalProb;
			_debugOut << " total: " << exp(newGuess->score);
			if (useRules) {
				if (_simpleRuleNameLinker->isMetonymyLinkCase(currMention, linkType, newGuess) &&
					!ParamReader::getRequiredTrueFalseParam("simple_rule_name_link_distillation") )
				{
					currMention->setMetonymyMention();
					newGuess->score = 1000;			
				}
				else if (!_simpleRuleNameLinker->isOKToLink(currMention, linkType, newGuess)) {
					newGuess->score = LOG_OF_ZERO;
				}
			}
			int j, k;
			// add results in sorted order
			bool insertedSolution = false;
			for (j=0; j<nResults; j++) {
				if(results[j]->score < newGuess->score) {
					if (nResults == max_results)
						delete results[max_results-1];
					else if (nResults < max_results)
						nResults++;
					for (k=nResults-1; k>j; k--)
						results[k] = results[k-1];
					results[j] = newGuess;
					newGuess = NULL;
					insertedSolution = true;
					break;
				}
			}
			// solution doesn't make the cut. if there's room, insert it at the end
			// otherwise, ditch it
			if (!insertedSolution) {
				if (nResults < max_results)
					results[nResults++] = newGuess;
				else
					delete newGuess;
				newGuess = NULL;
			}
		}
	}

//	_debugOut << "\n BEST: " << results[0]->id;
	return nResults;
}

