// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/GrowableArray.h"

#include "edt/NameLinker.h"
#include "common/ParamReader.h"
#include <math.h>
#include <stdio.h>
#include "edt/LexEntity.h"
#include "edt/AbbrevTable.h"
//note: the following header gives language specific functionality
#include "edt/NameLinkFunctions.h"
#include "theories/EntityConstants.h"
#include "common/UnrecoverableException.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8Token.h"
#include "common/SymbolConstants.h"
#include "common/SessionLogger.h"
#include <boost/scoped_ptr.hpp>

DebugStream &NameLinker::_debugOut = DebugStream::referenceResolverStream;

const double NameLinker::LOG_OF_ZERO = -10000;
NameLinker::NameLinker()
	: _genericModel(0), _newOldModel(0)
{
	_generic_unseen_weights = _new double[EntityConstants::all_size];
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	char filename[256];
	
	ParamReader::getNarrowParam(filename, Symbol(L"generic_model_file"), 256);
	instream.open(filename);
	_genericModel = _new ProbModel(2, instream, false);

	ParamReader::getNarrowParam(filename, Symbol(L"namelink_newold_model_file"), 256);
	instream.open(filename);
	_newOldModel = _new ProbModel(2, instream, false);

	loadWeights();
	
	AbbrevTable::initialize();

	_symOld = Symbol(L"OLD");
	_symNew = Symbol(L"NEW");
	
	_symNumbers[0] = Symbol(L"0");
	_symNumbers[1] = Symbol(L"1");
	_symNumbers[2] = Symbol(L"2");
	_symNumbers[3] = Symbol(L"3");
	_symNumbers[4] = Symbol(L"4");

	instream.close();
}

NameLinker::~NameLinker() {
	delete [] _generic_unseen_weights;
	AbbrevTable::destroy();
	NameLinkFunctions::destroyDataStructures();
	delete _genericModel;
	delete _newOldModel;
}

void NameLinker::cleanUpAfterDocument() {
	AbbrevTable::cleanUpAfterDocument();
}


void NameLinker::loadWeights() {
	//load generic and specific unseen weights
	SessionLogger* logger = SessionLogger::logger;

	// initialize the arrays with 0s
	int i;
	for (i=0; i < EntityConstants::all_size; i++) {
		LexEntity::unseen_weights[i] = 0;
		_generic_unseen_weights[i] = 0;
	}

	// notice the specific weights actually reside in LexEntity! 
	//The generic weights are local.
	char spec_file[256];
	bool result = ParamReader::getNarrowParam(spec_file, Symbol(L"specific_unseen_weights"), 256);
	if (!result)
		throw UnrecoverableException(
		"NameLinker::loadWeights()",
		"Parameter 'specific_unseen_weights' not specified");
	boost::scoped_ptr<UTF8InputStream> specStream_scoped_ptr(UTF8InputStream::build(spec_file));
	UTF8InputStream& specStream(*specStream_scoped_ptr);
	int n_entries = -1;
	specStream >> n_entries;
	if (specStream.eof() || n_entries < 0)
		throw UnrecoverableException(
		"NameLinker::loadWeights()",
		"Could not read specific-unseen-weights file!");
	UTF8Token token;
	if (n_entries != EntityConstants::all_size) {
		logger->beginWarning();
		*logger << "About to read in " << n_entries
			<< " specific weights; there have been " << EntityConstants::all_size
			<< " types read in.";
	}


	for (i=0; i < n_entries; i++) {
		// Expect paren, type, val, paren
		specStream >> token;
		if (token.symValue() != SymbolConstants::leftParen) {
			logger->beginError();
			*logger << "Expected '('; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
		specStream >> token;
		int pos = EntityConstants::getIndexOfType(token.symValue());
		if (pos < 0) {
			logger->beginWarning();
			*logger << "Entity type " << token.chars() << " not recognized";
		}
		double val;
		specStream >> val;
		if (pos >=0)
			LexEntity::unseen_weights[pos] = val;

		specStream >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			logger->beginError();
			*logger << "Expected ')'; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
	}
	specStream.close();

	char gen_file[256];
	result = ParamReader::getNarrowParam(gen_file, Symbol(L"generic_unseen_weights"), 256);
	if (!result)
		throw UnrecoverableException(
		"NameLinker::loadWeights()",
		"Parameter 'generic_unseen_weights' not specified");
	boost::scoped_ptr<UTF8InputStream> genStream_scoped_ptr(UTF8InputStream::build(gen_file));
	UTF8InputStream& genStream(*genStream_scoped_ptr);
	n_entries = -1;
	genStream >> n_entries;
	if (genStream.eof() || n_entries < 0)
		throw UnrecoverableException(
		"NameLinker::loadWeights()",
		"Could not read generic-unseen-weights file!");
	if (n_entries != EntityConstants::all_size) {
		logger->beginWarning();
		*logger << "About to read in " << n_entries
			<< " generic weights; there have been " << EntityConstants::all_size
			<< " types read in.";
	}

	for (i=0; i < n_entries; i++) {
		// Expect paren, type, val, paren
		genStream >> token;
		if (token.symValue() != SymbolConstants::leftParen) {
			logger->beginError();
			*logger << "Expected '('; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
		genStream >> token;
		int pos = EntityConstants::getIndexOfType(token.symValue());
		if (pos < 0) {
			logger->beginWarning();
			*logger << "Entity type " << token.chars() << " not recognized";
		}
		double val;
		genStream >> val;
		if (pos >=0)
			_generic_unseen_weights[pos] = val;

		genStream >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			logger->beginError();
			*logger << "Expected ')'; read in " << token.chars()
				<< "; Aborting file";
			break;
		}
	}
	genStream.close();
}

int NameLinker::linkMention (LexEntitySet * currSolution, int currMentionUID, Entity::Type linkType, LexEntitySet *results[], int max_results) {
	//retrieve the guesses for this mention, populate array with forked EntitySets, return array
	LexEntitySet * newSet = NULL;
	EntityGuess *guesses [64];
	Mention *currMention = currSolution->getMention(currMentionUID);
	int nGuesses = 0;
	_debugOut << "\nProcessing mention #" << currMention->getUID() << "\n";
	//DEBUG: print out all symbols, and next just head's symbols
	Symbol symArrayAll[16], symArrayHead[16], symArrayMentionHead[16];
	int nAll, nHead, nMentionHead;
	nAll = currMention->node->getTerminalSymbols(symArrayAll, 16);
	nHead = currMention->node->getHead()->getTerminalSymbols(symArrayHead, 16);
	nMentionHead = currMention-> getHead()->getTerminalSymbols(symArrayMentionHead, 16);
	int i;
	//_debugOut << "\nMention head symbols: ";
	//for (i=0; i<nMentionHead; i++) 
		//_debugOut << symArrayMentionHead[i].to_string() << " "; //Marj changed from debug_string
	//_debugOut << "\n";
	//END DEBUG
	nGuesses = guessEntity(currSolution, currMention, linkType, guesses, max_results>64 ? 64: max_results);

	//int i;
	for (i = 0; i<nGuesses; i++) {
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
		newSet->setScore(newSet->getScore()+thisGuess->score);
		results[i] = newSet;
		delete thisGuess;
		thisGuess = guesses[i] = NULL;
		newSet->customDebugPrint(_debugOut);
	}
	_debugOut << "\nDone processing mention #" << currMention->getIndex() << "\n\n";
	
	return nGuesses;	
}

int NameLinker::guessEntity(LexEntitySet * currSolution, Mention * currMention, Entity::Type linkType, EntityGuess *results[], int max_results) {
	_debugOut << "BEGIN_GUESSES\n";
	
	//this guessing algorithm relies on our already having resolved the type of the mention
	EntityGuess * newGuess = _new EntityGuess();
	int numEntities, numEntitiesNorm;
	Symbol words[32], resolved[32], lexicalItems[32]; 
	int nWords, nResolved, nLexicalItems, nResults = 0;
	Symbol trans[2];

	numEntities = currSolution->getNEntities();
	numEntitiesNorm = numEntities>4 ? 4 : numEntities;

	//populate the dynamic abbreviations table if necessary
	NameLinkFunctions::populateAbbrevs(currMention, linkType);
	
	//first score the new entity option
	_debugOut << "NEW: " << linkType.to_debug_string();
	newGuess->id = EntityGuess::NEW_ENTITY;
	newGuess->type = linkType; 

	trans[0] = _symNumbers[numEntitiesNorm];
	trans[1] = _symNew;
	newGuess->score = _newOldModel->getProbability(trans);
	_debugOut << " N/O: " << exp(newGuess->score);
	int i;
	nWords = currMention->getHead()->getTerminalSymbols(words, 16);
	nResolved = AbbrevTable::resolveSymbols(words, nWords, resolved, 32);
	nLexicalItems = NameLinkFunctions::getLexicalItems(resolved, nResolved, lexicalItems, 32);

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
		trans[0] = newGuess->type;
		trans[1] = lexicalItems[i];
		int index = EntityConstants::getIndexOfType(newGuess->type);
		double lambda = _generic_unseen_weights[index];
		double thisWordScore = log (
			(1-lambda) * exp(_genericModel->getProbability(trans)) + lambda * 1.0/10000);
		newGuess->score += thisWordScore;
		_debugOut << " [" << lexicalItems[i].to_debug_string() << ", " << exp(thisWordScore) << "] ";
	}
	_debugOut << "total: " << exp (newGuess->score);
	results[0] = newGuess;
	nResults++;

	
	//now score each of the possible linkings
	GrowableArray <Entity *> &filteredEnts = currSolution->getEntitiesByType(linkType);
	for (i=0; i<filteredEnts.length(); i++) {
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
//	_debugOut << "\n BEST: " << results[0]->id;
	return nResults;
}
