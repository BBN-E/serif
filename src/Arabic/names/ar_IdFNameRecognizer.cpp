// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/IdFDecoder.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/SessionLogger.h"
#include "Arabic/names/ar_IdFNameRecognizer.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/names/IdFSentence.h"
#include "Arabic/BuckWalter/ar_Retokenizer.h"

#define MAX_IDF_THEORIES 30
ArabicIdFNameRecognizer::ArabicIdFNameRecognizer()  {

	std::string debug_buffer = ParamReader::getRequiredParam("name_debug");
	DEBUG = (debug_buffer != "");
	if (DEBUG) _debugStream.open(debug_buffer.c_str());

	std::string model_prefix = ParamReader::getRequiredParam("idf_model");
	std::string name_class_tags = ParamReader::getParam("idf_name_class");
	if (!name_class_tags.empty())
		_nameClassTags = _new NameClassTags(name_class_tags.c_str());
	else
		_nameClassTags = _new NameClassTags();
	
	_separateClitics = ParamReader::isParamTrue("idf_separate_all_clitics");

	string list_file = ParamReader::getParam("idf_list_file");
	if (!list_file.empty())
		_decoder = _new IdFDecoder(model_prefix.c_str(), _nameClassTags, list_file.c_str());
	else 
		_decoder = _new IdFDecoder(model_prefix.c_str(), _nameClassTags);

	_idfToks = _new IdFTokens();
	//if (!_decoder->checkNameClassTagConsistency())
	//	throw UnexpectedInputException("ArabicNameRecognizer::ArabicNameRecognizer()",
	//								   "model tags inconsistent with name class tags");
	_sentenceTokens = _new IdFSentenceTokens();
	_IdFTheories = _new IdFSentenceTheory* [MAX_IDF_THEORIES];
}

void ArabicIdFNameRecognizer::resetForNewSentence() {
}

int ArabicIdFNameRecognizer::getNameTheories(NameTheory **results, int max_theories,
									TokenSequence *tokenSequence)
{
	LexicalTokenSequence *lexicalTokenSequence = dynamic_cast<LexicalTokenSequence*>(tokenSequence);
	if (!lexicalTokenSequence) throw InternalInconsistencyException("ArabicIdFNameRecognizer::getNameTheories",
		"This ArabicIdFNameRecognizer requires a LexicalTokenSequence.");

	if (max_theories > 1) {
		throw UnexpectedInputException("ArabicNameRecognizer::getNameTheories()",
			"Name finder branching factor is not 1. The name finder does not\n"
			"currently support multiple theories.");
		// this is because it must modify the token sequence. -- SRS
	}
	if (max_theories > MAX_IDF_THEORIES)
		max_theories = MAX_IDF_THEORIES;
	int n_tokens;
	//Do IdF tokenization
	if(_separateClitics){
		_idfToks->tokenize(lexicalTokenSequence);
		n_tokens = _idfToks->getNumTokens();
		if (n_tokens > MAX_SENTENCE_TOKENS)
			n_tokens = MAX_SENTENCE_TOKENS;
		//put tokens into idfSentence
		for(int i =0; i< n_tokens; i++){
			_sentenceTokens->setWord(i, _idfToks->getToken(i)->getSymbol());
			if(DEBUG){
				Symbol dSym = _idfToks->getToken(i)->getSymbol();
				_debugStream <<dSym.to_debug_string()<<L" ";
				int realIndex = _idfToks->getSerifTokenNum(i);
				dSym = lexicalTokenSequence->getToken(realIndex)->getSymbol();
				_debugStream <<dSym.to_debug_string()<<L" \n";
			}
		}
	}
	else{
		n_tokens = lexicalTokenSequence->getNTokens();
		for (int i = 0; i < n_tokens; i++) {
			_sentenceTokens->setWord(i, lexicalTokenSequence->getToken(i)->getSymbol());
		}
	}

	_sentenceTokens->setLength(n_tokens);

    //decode
	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens,
			_IdFTheories, max_theories);
	if (num_theories == 0) {
		// should never happen, but...
		_IdFTheories[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}


	if (DEBUG)  {
		_debugStream << _nameClassTags->to_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n";
		_debugStream << _nameClassTags->to_enamex_sgml_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n\n";
	}

	//make a Name Theory
	if(!_separateClitics){
		int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[0]);

		NameTheory* pTheory;

		pTheory = _new NameTheory(lexicalTokenSequence, name_count);
		pTheory->setScore(static_cast<float>(_IdFTheories[0]->getBestPossibleScore()));
		bool in_name = false;
		int name_index = 0;
		int i = 0;
		for (i = 0; i < n_tokens; i++) {
			int tag = _IdFTheories[0]->getTag(i);

			if (_nameClassTags->isStart(tag) && in_name) {
				pTheory->getNameSpan(name_index)->end = i - 1;
				name_index++;
				in_name = false;
			}
			if (_nameClassTags->isStart(tag) &&
				_nameClassTags->isMeaningfulNameTag(tag))
			{
				//need to switch from idfTokenization, back to original tokens
				// Note: the span end, which we set to -1 here, will be given a value
				// during a subsequent pass through the loop (when in_name=true).
				pTheory->takeNameSpan(
					_new NameSpan(i, -1, _nameClassTags->getReducedTagSymbol(tag)));
				in_name = true;
			}
		}
		if (in_name) {
			pTheory->getNameSpan(name_index)->end = i - 1;
			name_index++;
			in_name = false;
		}
		results[0] = pTheory;


		return 1;
	}

	if (DEBUG)
		_debugStream << _nameClassTags->to_sgml_string(_sentenceTokens,
			_IdFTheories[0]) << L"\n";
	int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[0]);

	NameTheory* pTheory;

	pTheory = _new NameTheory(lexicalTokenSequence, name_count);
	pTheory->setScore(static_cast<float>(_IdFTheories[0]->getBestPossibleScore()));

	// populate array of bools which tells us whether to split each token
	// or keep the original token
	bool split_old_token[MAX_SENTENCE_TOKENS];
	for (int j = 0; j < MAX_SENTENCE_TOKENS; j++)
		split_old_token[j] = false;
	int i = 0;
	for (i = 0; i < n_tokens; i++) {
		int tag = _IdFTheories[0]->getTag(i);

		if ((_nameClassTags->isStart(tag) &&
			 _nameClassTags->isMeaningfulNameTag(tag)) &&
			token_smaller_than_serif_token(i))
		{
			split_old_token[_idfToks->getSerifTokenNum(i)] = true;
		}
	}

	// create mapping from grainy tokens to new tokens
	int token_mapping[MAX_SENTENCE_TOKENS];
	int token_index = -1;
	int prev_old_tok = -1;
	for (int k = 0; k < n_tokens; k++) {
		int old_tok = _idfToks->getSerifTokenNum(k);
		if (old_tok == prev_old_tok) {
			if (split_old_token[old_tok])
				token_index++;
		}
		else {
			token_index++;
			prev_old_tok = old_tok;
		}

		token_mapping[k] = token_index;
	}
	int n_new_toks = token_index + 1;


	Token *newSequence[MAX_SENTENCE_TOKENS];
	int name_index = 0;
	int prev_new_tok = -1;
	bool in_name = false;
	for (int m = 0; m < n_tokens; m++) {
		int tag = _IdFTheories[0]->getTag(m);

		if (_nameClassTags->isStart(tag) && in_name) {
			pTheory->getNameSpan(name_index)->end = token_mapping[m - 1];
			name_index++;
			in_name = false;
		}
		if (_nameClassTags->isStart(tag) &&
			_nameClassTags->isMeaningfulNameTag(tag))
		{
			//need to switch from idfTokenization, back to original tokens
			// Note: the span end, which we set to -1 here, will be given a value
			// during a subsequent pass through the loop (when in_name=true).
			pTheory->takeNameSpan(
				_new NameSpan(token_mapping[m], -1, _nameClassTags->getReducedTagSymbol(tag)));
			in_name = true;
		}

		int new_tok = token_mapping[m];
		if (new_tok >= MAX_SENTENCE_TOKENS)
			break;

		if (new_tok != prev_new_tok) {
			prev_new_tok = new_tok;

			int old_tok = _idfToks->getSerifTokenNum(m);
			if (split_old_token[old_tok]) {
				newSequence[new_tok] = _new LexicalToken(*_idfToks->getToken(m));
			}
			else {
				const LexicalToken *curToken = lexicalTokenSequence->getToken(
					_idfToks->getSerifTokenNum(m));
				newSequence[new_tok] = _new LexicalToken(*curToken);
			}
		}
	}
	if (in_name) {
		//need to switch from idfTokenization, back to original tokens
		//pTheory->getNameSpan(name_index)->end = n_tokens - 1;
		pTheory->getNameSpan(name_index)->end = token_mapping[i - 1];
		name_index++;
		in_name = false;
	}
	results[0] = pTheory;

	// this is the nasty part
	//lexicalTokenSequence->retokenize(n_new_toks, newSequence);
	//use retokenizer to recapture lexical entry information

	Retokenizer::getInstance().RetokenizeForNames(lexicalTokenSequence, newSequence, n_new_toks);
	//now delete the new tokens
	for(int d=0; d< n_new_toks; d++){
		delete newSequence[d];
	}



	if (DEBUG) _debugStream << "\n";
	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		delete _IdFTheories[theorynum];
	}
	_idfToks->resetTokens();	//should this go in resetForNewSentence?

	return 1;
}


int ArabicIdFNameRecognizer::getNameTheoriesForSentenceBreaking(NameTheory **results, int max_theories,
									TokenSequence *tokenSequence)
{
	LexicalTokenSequence *lexicalTokenSequence = dynamic_cast<LexicalTokenSequence*>(tokenSequence);
	if (!lexicalTokenSequence) throw InternalInconsistencyException("ArabicIdFNameRecognizer::getNameTheories",
		"This ArabicIdFNameRecognizer requires a LexicalTokenSequence.");

	if (max_theories > 1) {
		throw UnexpectedInputException("ArabicNameRecognizer::getNameTheories()",
			"Name finder branching factor is not 1. The name finder does not\n"
			"currently support multiple theories.");
		// this is because it must modify the token sequence. -- SRS
	}

	if (max_theories > MAX_IDF_THEORIES)
		max_theories = MAX_IDF_THEORIES;
	int n_tokens;
	//Do IdF tokenization
	if(_separateClitics){
		_idfToks->tokenize(lexicalTokenSequence);
		n_tokens = _idfToks->getNumTokens();
		if (n_tokens > MAX_SENTENCE_TOKENS)
			n_tokens = MAX_SENTENCE_TOKENS;
		//put tokens into idfSentence
		for(int i =0; i< n_tokens; i++){
			_sentenceTokens->setWord(i, _idfToks->getToken(i)->getSymbol());
			if(DEBUG){
				Symbol dSym = _idfToks->getToken(i)->getSymbol();
				_debugStream <<dSym.to_debug_string()<<L" ";
				int realIndex = _idfToks->getSerifTokenNum(i);
				dSym = lexicalTokenSequence->getToken(realIndex)->getSymbol();
				_debugStream <<dSym.to_debug_string()<<L" \n";
			}
		}
	}
	else{
		n_tokens = lexicalTokenSequence->getNTokens();
		for (int i = 0; i < n_tokens; i++) {
			_sentenceTokens->setWord(i, lexicalTokenSequence->getToken(i)->getSymbol());
		}
	}

	_sentenceTokens->setLength(n_tokens);

    //decode
	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens,
			_IdFTheories, max_theories);
	if (num_theories == 0) {
		// should never happen, but...
		_IdFTheories[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}


	if (DEBUG)  {
		_debugStream << _nameClassTags->to_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n";
		_debugStream << _nameClassTags->to_enamex_sgml_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n\n";
	}

	//make a Name Theory
	if(!_separateClitics){
		int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[0]);

		NameTheory* pTheory;

		pTheory = _new NameTheory(lexicalTokenSequence, name_count);
		pTheory->setScore(static_cast<float>(_IdFTheories[0]->getBestPossibleScore()));
		bool in_name = false;
		int name_index = 0;
		int i = 0;
		for (int i = 0; i < n_tokens; i++) {
			int tag = _IdFTheories[0]->getTag(i);

			if (_nameClassTags->isStart(tag) && in_name) {
				pTheory->getNameSpan(name_index)->end = i - 1;
				name_index++;
				in_name = false;
			}
			if (_nameClassTags->isStart(tag) &&
				_nameClassTags->isMeaningfulNameTag(tag))
			{
				// Note: the span end, which we set to -1 here, will be given a value
				// during a subsequent pass through the loop (when in_name=true).
				//need to switch from idfTokenization, back to original tokens
				pTheory->takeNameSpan(
					_new NameSpan(i, -1, _nameClassTags->getReducedTagSymbol(tag)));
				in_name = true;
			}
		}
		if (in_name) {
			pTheory->getNameSpan(name_index)->end = i - 1;
			name_index++;
			in_name = false;
		}
		results[0] = pTheory;


		return 1;
	}

	if (DEBUG)
		_debugStream << _nameClassTags->to_sgml_string(_sentenceTokens,
			_IdFTheories[0]) << L"\n";
	int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[0]);

	NameTheory* pTheory;

	pTheory = _new NameTheory(lexicalTokenSequence, name_count);
	pTheory->setScore(static_cast<float>(_IdFTheories[0]->getBestPossibleScore()));

	// populate array of bools which tells us whether to split each token
	// or keep the original token
	bool split_old_token[MAX_SENTENCE_TOKENS];
	for (int j = 0; j < MAX_SENTENCE_TOKENS; j++)
		split_old_token[j] = false;
	int i = 0;
	for (i = 0; i < n_tokens; i++) {
		int tag = _IdFTheories[0]->getTag(i);

		if ((_nameClassTags->isStart(tag) &&
			 _nameClassTags->isMeaningfulNameTag(tag)) &&
			token_smaller_than_serif_token(i))
		{
			split_old_token[_idfToks->getSerifTokenNum(i)] = true;
		}
	}

	// create mapping from grainy tokens to new tokens
	int token_mapping[MAX_SENTENCE_TOKENS];
	int token_index = -1;
	int prev_old_tok = -1;
	for (int k = 0; k < n_tokens; k++) {
		int old_tok = _idfToks->getSerifTokenNum(k);
		if (old_tok == prev_old_tok) {
			if (split_old_token[old_tok])
				token_index++;
		}
		else {
			token_index++;
			prev_old_tok = old_tok;
		}

		token_mapping[k] = token_index;
	}
	int n_new_toks = token_index + 1;


	Token *newSequence[MAX_SENTENCE_TOKENS];
	int name_index = 0;
	int prev_new_tok = -1;
	bool in_name = false;
	for (int m = 0; m < n_tokens; m++) {
		int tag = _IdFTheories[0]->getTag(m);

		if (_nameClassTags->isStart(tag) && in_name) {
			pTheory->getNameSpan(name_index)->end = token_mapping[m - 1];
			name_index++;
			in_name = false;
		}
		if (_nameClassTags->isStart(tag) &&
			_nameClassTags->isMeaningfulNameTag(tag))
		{
			// Note: the span end, which we set to -1 here, will be given a value
			// during a subsequent pass through the loop (when in_name=true).
			//need to switch from idfTokenization, back to original tokens
			pTheory->takeNameSpan(
				_new NameSpan(token_mapping[m], -1, _nameClassTags->getReducedTagSymbol(tag)));
			in_name = true;
		}

		int new_tok = token_mapping[m];
		if (new_tok >= MAX_SENTENCE_TOKENS)
			break;

		if (new_tok != prev_new_tok) {
			prev_new_tok = new_tok;

			int old_tok = _idfToks->getSerifTokenNum(m);
			if (split_old_token[old_tok]) {
				newSequence[new_tok] = _new LexicalToken(*_idfToks->getToken(m));
			}
			else {
				const LexicalToken *curToken = lexicalTokenSequence->getToken(
					_idfToks->getSerifTokenNum(m));
				newSequence[new_tok] = _new LexicalToken(*curToken);
			}
		}
	}
	if (in_name) {
		//need to switch from idfTokenization, back to original tokens
		//pTheory->getNameSpan(name_index)->end = n_tokens - 1;
		pTheory->getNameSpan(name_index)->end = token_mapping[i - 1];
		name_index++;
		in_name = false;
	}
	results[0] = pTheory;

	// this is the nasty part
	//lexicalTokenSequence->retokenize(n_new_toks, newSequence);
	//use retokenizer to recapture lexical entry information

	Retokenizer::getInstance().RetokenizeForNames(lexicalTokenSequence, newSequence, n_new_toks);
	//now delete the new tokens
	//for(int d=0; d< n_new_toks; d++){
	//	delete newSequence[d];
	//}



	if (DEBUG) _debugStream << "\n";
	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		delete _IdFTheories[theorynum];
	}
	_idfToks->resetTokens();	//should this go in resetForNewSentence?

	return 1;
}







bool ArabicIdFNameRecognizer::token_smaller_than_serif_token(int i) {
	if (i == 0)
		return false;
	if (_idfToks->getSerifTokenNum(i - 1) == _idfToks->getSerifTokenNum(i))
		return true;
	else
		return false;
}
