// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/names/ch_IdFNameRecognizer.h"
#include "Chinese/names/ch_NameRecognizer.h"
#include "Chinese/common/UnicodeGBTranslator.h"
#include "Chinese/common/ch_StringTransliterator.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Chinese/tokens/ch_Tokenizer.h"
#include "Chinese/tokens/ch_Untokenizer.h"
#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/names/IdFDecoder.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NameTheory.h"

#define MAX_IDF_THEORIES 30

using namespace std;

ChineseIdFNameRecognizer::ChineseIdFNameRecognizer() {

	// read params
	std::string model_prefix = ParamReader::getRequiredParam("idf_params");
	
	std::string idf_mode = ParamReader::getParam("idf_mode");
	if (idf_mode == "tokens") 
		_run_on_tokens = true;
	else
		_run_on_tokens = false;

	_nameClassTags = _new NameClassTags();
	_decoder = _new IdFDecoder(model_prefix.c_str(), _nameClassTags);
	_IdFTheories = _new IdFSentenceTheory* [MAX_IDF_THEORIES];
	_sentenceTokens = _new IdFSentenceTokens();
}

void ChineseIdFNameRecognizer::resetForNewSentence(const Sentence *sentence) {
}

int ChineseIdFNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence) 
{
	if( _run_on_tokens ) 
		return _getNameTheoriesRunOnTokens(results, max_theories, tokenSequence);
	else
		return _getNameTheoriesRunOnKanji(results, max_theories, tokenSequence);
}

int ChineseIdFNameRecognizer::_getNameTheoriesRunOnKanji(NameTheory **results, int max_theories, 
									const TokenSequence *tokenSequence)
{
	if (max_theories > MAX_IDF_THEORIES)
		max_theories = MAX_IDF_THEORIES;

	SessionLogger &logger = *SessionLogger::logger;
	int i, char_count = 0;
	NameTheory *pTheory;
	NameSpan *prevSpan = 0;
	int n_tokens = tokenSequence->getNTokens();
	float score_penalty;

	if (max_theories < 1) {
		SessionLogger::warn("idf") << "ChineseIdFNameRecognizer::getNameTheories(), max_theories is less than one.";
		return 0;
	}
	
	
	// mapping from character (IdFToken) to Serif token
	int token_cache[MAX_CHARACTERS_PER_SENTENCE]; 

	// make a sentence out of all the wchars and decode
	_char_buffer[1] = L'\0';
	for (i = 0; i < n_tokens; i++) {
		Symbol pToken = tokenSequence->getToken(i)->getSymbol();
		ChineseUntokenizer::untokenize(pToken.to_string(), _word_buffer, MAX_TOKEN_SIZE+1);
		int j = 0;
		while (_word_buffer[j] != L'\0') {
			// set buffer to the character
			_char_buffer[0] = _word_buffer[j];
			_sentenceTokens->setWord(char_count, ChineseTokenizer::getSubstitutionSymbol(Symbol(_char_buffer)));
			// record which token the character comes from
			token_cache[char_count] = i;
			char_count++;
			j++;
		}
	}
	_sentenceTokens->setLength(char_count);

	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens, 
		_IdFTheories, max_theories);
	
	if (num_theories == 0) {
		// should never happen, but...
		_IdFTheories[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}
	
	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		score_penalty = 0.0;
		int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[theorynum]);
		if (name_count > MAX_NAMES) {
			SessionLogger::warn("idf") << "Warning: ChineseIdFNameRecognizer::getNameTheories() "
				   << "theory has more than MAX_NAMES names. Truncating\n";
			name_count = MAX_NAMES;
		}
	
		// create spans from _sentenceTheory and token_cache
		int name_index = 0;
		int last_end_char = -1;
		bool in_name = false;
		for (i = 0; i < char_count; i++) {
			int tag = _IdFTheories[theorynum]->getTag(i);
			if (_nameClassTags->isStart(tag) && name_index < MAX_NAMES) {
				if (in_name) {
					_spanBuffer[name_index]->end = token_cache[i - 1];
					
					if (token_cache[i - 1] == token_cache[i])
						score_penalty += 100.0;

					name_index++;
					in_name = false;
					last_end_char = i - 1;
				}
				if (_nameClassTags->isMeaningfulNameTag(tag)) {
					if (token_cache[i] == token_cache[i - 1])
						score_penalty += 100.0;

					_spanBuffer[name_index] = _new NameSpan();
					_spanBuffer[name_index]->start = token_cache[i];
					_spanBuffer[name_index]->type = _nameClassTags->getReducedTagSymbol(_IdFTheories[theorynum]->getTag(i));
				
					if (i > 0 && last_end_char != -1 && token_cache[i] < token_cache[last_end_char]) 
						throw UnrecoverableException
							("ChineseIdFNameRecognizer::getNameTheories", "token_cache inconsistent");

					if (i > 0 && last_end_char != -1 && token_cache[i] == token_cache[last_end_char]) {
	
						//logger.beginWarning();
						//logger << "Warning: ChineseIdFNameRecognizer::getNameTheories(), overlapping names. Adjusting name offsets\n";
					
						// we have overlap, figure out which name should get the token, token_cache[i]
						if (firstNameHasRightsToToken(last_end_char, i, token_cache)) {
							_spanBuffer[name_index]->start = _spanBuffer[name_index]->start + 1;
						} else {
							_spanBuffer[name_index - 1]->end = _spanBuffer[name_index - 1]->end - 1;
						}
					}

					in_name = true;
				}
			}
		}
		if (in_name) {
			_spanBuffer[name_index]->end = token_cache[char_count - 1];						
			name_index++;
			if (name_index >= MAX_NAMES) {
				SessionLogger::warn("idf") << "ch_ChineseIdFNameRecognizer::getNameTheoriesRunOnKanji(): " 
						<< "reached MAX_NAMES in this sentence - no more names will be added\n";
			}
			in_name = false;
		}

		// not all these names will be good, some will now have zero length, or overlap previous name
		int bad_name_count = 0;
		int last_name_end = -1;
		for (i = 0; i < name_count; i++ ) {
			if (_spanBuffer[i]->start <= last_name_end || 
				_spanBuffer[i]->start > _spanBuffer[i]->end ) {

				//logger.beginWarning();
				//logger << "Warning: ChineseIdFNameRecognizer::getNameTheories(), zero length name. Removing.\n";
				_spanBuffer[i]->start = OVERLAPPING_SPAN;
				_spanBuffer[i]->end = OVERLAPPING_SPAN;
				bad_name_count++;
			} else
				last_name_end = _spanBuffer[i]->end;
		}
		int true_name_count = name_count - bad_name_count;

		pTheory = _new NameTheory(tokenSequence);
		pTheory->setScore(static_cast<float>(_IdFTheories[theorynum]->getBestPossibleScore() - score_penalty));
		for (i = 0; i < name_count; i++) {
			if (_spanBuffer[i]->start != OVERLAPPING_SPAN) {
				pTheory->takeNameSpan(_spanBuffer[i]);
			}
			else
				delete _spanBuffer[i]; 
		}
		results[theorynum] = pTheory;
	}
	
	for (int theory_num = 0; theory_num < num_theories; theory_num++) {
		delete _IdFTheories[theory_num];
	}

	return num_theories;

}

int ChineseIdFNameRecognizer::_getNameTheoriesRunOnTokens(NameTheory **results, int max_theories, 
									const TokenSequence *tokenSequence)
{
	if (max_theories > MAX_IDF_THEORIES)
		max_theories = MAX_IDF_THEORIES;

	SessionLogger &logger = *SessionLogger::logger;
	int i, char_count = 0;
	NameSpan *prevSpan = 0;
	int n_tokens = tokenSequence->getNTokens();

	const Token* pToken;
	Symbol symbol;

	if (max_theories < 1) {
		SessionLogger::warn("idf") << "ChineseIdFNameRecognizer::getNameTheories(), max_theories is less than one.";
		return 0;
	}

	for (i = 0; i < n_tokens; i++) {
		pToken = tokenSequence->getToken(i);
		_sentenceTokens->setWord(i, pToken->getSymbol());
	}
	_sentenceTokens->setLength(n_tokens);

	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens, 
		_IdFTheories, max_theories);
	
	if (num_theories == 0) {
		// should never happen, but...
		_IdFTheories[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}
	
	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[theorynum]);

		NameTheory* pTheory;

		pTheory = _new NameTheory(tokenSequence);
		pTheory->setScore(static_cast<float>(_IdFTheories[theorynum]->getBestPossibleScore()));

		int name_index = 0;
		bool in_name = false;
		for (int i = 0; i < n_tokens; i++) {
			int tag = _IdFTheories[theorynum]->getTag(i);
			if (_nameClassTags->isStart(tag)) {
				if (in_name) {
					pTheory->getNameSpan(name_index)->end = i - 1;									
					name_index++;
					in_name = false;
				}
				if (_nameClassTags->isMeaningfulNameTag(tag)) {	
					// name span's end is set in subsequent pass through the loop
					pTheory->takeNameSpan(
						_new NameSpan(i, -1, _nameClassTags->getReducedTagSymbol(_IdFTheories[theorynum]->getTag(i))));
					in_name = true;
				}
			}
		}
		if (in_name) {
			pTheory->getNameSpan(name_index)->end = n_tokens - 1;									
			name_index++;
			in_name = false;
		}

		results[theorynum] = pTheory;
	}
	
	for (int theory_num = 0; theory_num < num_theories; theory_num++) {
		delete _IdFTheories[theory_num];
	}

	return num_theories;

}

bool ChineseIdFNameRecognizer::firstNameHasRightsToToken(int first_token_char_index, int second_token_char_index, int* token_cache) 
{
	int start_token_index = token_cache[first_token_char_index];
	for (int i = 0; i < MAX_CHARACTERS_PER_SENTENCE - first_token_char_index; i++ ) {
		if (token_cache[second_token_char_index + i] != start_token_index) 
			return true;
		if (first_token_char_index - i < 0)
			return true;
		if (token_cache[first_token_char_index - i] != start_token_index) 
			return false;
	}
	return false;
}
