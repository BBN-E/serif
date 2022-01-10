// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/morphSelection/MorphDecoder.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/morphSelection/xx_MorphDecoder.h"
#include "Generic/morphSelection/MorphModel.h"
#include "Generic/morphSelection/ParseSeeder.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"


MorphDecoder::MorphDecoder() : model(0), ps(0) {
	std::string buffer = ParamReader::getRequiredParam("MorphModel");
	model = MorphModel::build(buffer.c_str());
	ps = ParseSeeder::build();
	_num_tokens = 0;
}

MorphDecoder::~MorphDecoder() {
	delete ps;
	delete model;
}

int MorphDecoder::getBestWordSequence(const LocatedString& sentenceString,
                                             TokenSequence* ts,
                                             Symbol* word_sequence, 
                                             int max_words)
{
	putTokenSequenceInAtomizedSentence(sentenceString, ts);
	int best = walkForwardThroughSentence();
	int final_size = 0;
	int this_best = best;
	//get the number of tokens in the final segment
	for (int n = ts->getNTokens()-1; n >= 0; n--) {
		final_size += static_cast<int>(_atomized_sentence[n].possibilities[this_best].size());
		this_best = _words[n][this_best].prev_poss_index;
	}
	if (final_size > max_words) {
		char errorString[200];
		sprintf(errorString, "WordSequence with %d tokens exceeds max_words of %d!", final_size, max_words);
		throw UnrecoverableException("MorphDecoder::getBestWordSequence()", errorString);
	}

	int word_num = final_size - 1;
	this_best = best;
	for (int i = ts->getNTokens()-1; i >= 0; i--) {
		int num_seg = static_cast<int>(_atomized_sentence[i].possibilities[this_best].size());
		for (int j = num_seg-1; j >= 0; j--) {
			word_sequence[word_num] = _atomized_sentence[i].possibilities[this_best][j].symbol;
			word_num--;
		}
		this_best = _words[i][this_best].prev_poss_index;
	}
//	PrintTrellis();
	return final_size;
}

int MorphDecoder::getBestWordSequence(const LocatedString& sentenceString, TokenSequence* ts, Symbol* word_sequence,
									  int* map, int max_words) 
{
	putTokenSequenceInAtomizedSentence(sentenceString, ts);
	int best = walkForwardThroughSentence();
	int final_size = 0;
	int this_best = best;

	//count the number of tokens in the final segment
	for (int n = ts->getNTokens()-1; n >= 0; n--) {
		final_size += static_cast<int>(_atomized_sentence[n].possibilities[this_best].size());
		this_best = _words[n][this_best].prev_poss_index;
	}
	if (final_size > max_words) {
		char errorString[200];
		sprintf(errorString, "WordSequence with %d tokens exceeds max_words of %d!", final_size, max_words);
		throw UnrecoverableException("MorphDecoder::getBestWordSequence()", errorString);
	}


	int word_num = final_size - 1;
	this_best = best;
	for (int i = ts->getNTokens()-1; i >= 0; i--) {
		int num_seg = static_cast<int>(_atomized_sentence[i].possibilities[this_best].size());
		for (int j = num_seg-1; j>= 0; j--) {
			word_sequence[word_num] = _atomized_sentence[i].possibilities[this_best][j].symbol;
			map[word_num] = i;
			word_num--;
		}
		this_best = _words[i][this_best].prev_poss_index;
	}
//	PrintTrellis();
	return final_size;
}


int MorphDecoder::getBestWordSequence(const LocatedString& sentenceString, TokenSequence*ts, Symbol* word_sequence, 
									  int* map, OffsetGroup* start, OffsetGroup* end, int max_words) 
{
	putTokenSequenceInAtomizedSentence(sentenceString, ts);
	int best = walkForwardThroughSentence();
	int final_size = 0;
	int this_best = best;
	
	//count the number of tokens in the final segment
	for (int n = ts->getNTokens()-1; n >= 0; n--) {
		final_size += static_cast<int>(_atomized_sentence[n].possibilities[this_best].size());
		this_best = _words[n][this_best].prev_poss_index;
	}
	if (final_size > max_words) {
		char errorString[200];
		sprintf(errorString, "WordSequence with %d tokens exceeds max_words of %d!", final_size, max_words);
		throw UnrecoverableException("MorphDecoder::getBestWordSequence()", errorString);
	}


	int word_num = final_size - 1;
	this_best = best;
	for (int i = ts->getNTokens()-1; i >= 0; i--) {
		int num_seg = static_cast<int>(_atomized_sentence[i].possibilities[this_best].size());
		for (int j = num_seg-1; j >= 0; j--) {
			word_sequence[word_num] = _atomized_sentence[i].possibilities[this_best][j].symbol;
			start[word_num] = _atomized_sentence[i].possibilities[this_best][j].start;
			end[word_num] = _atomized_sentence[i].possibilities[this_best][j].end;
			map[word_num] = i;
			word_num--;
		}
		this_best = _words[i][this_best].prev_poss_index;
	}
//	PrintTrellis();
	return final_size;
}

int MorphDecoder::getBestWordSequence(const LocatedString& sentenceString, TokenSequence*ts, Symbol* word_sequence, int* map,
		OffsetGroup* start, OffsetGroup* end, CharOffset* constraints, int n_constraints, int max_words)
{
	putTokenSequenceInAtomizedSentence(sentenceString, ts, constraints, n_constraints);
	int best = walkForwardThroughSentence();
	int final_size = 0;
	int this_best = best;
	
	//get the number of tokens in the final segment
	for (int n = ts->getNTokens() - 1; n >= 0; n--) {
		final_size += static_cast<int>(_atomized_sentence[n].possibilities[this_best].size());
		this_best = _words[n][this_best].prev_poss_index;
	}
	if (final_size > max_words) {
		char errorString[200];
		sprintf(errorString, "WordSequence with %d tokens exceeds max_words of %d!", final_size, max_words);
		throw UnrecoverableException("MorphDecoder::getBestWordSequence()", errorString);
	}

	int word_num = final_size - 1;
	this_best = best;
	for (int i = ts->getNTokens()-1; i >= 0; i--) {
		int num_seg = static_cast<int>(_atomized_sentence[i].possibilities[this_best].size());
		for (int j = num_seg-1; j >= 0; j--) {
			word_sequence[word_num] = _atomized_sentence[i].possibilities[this_best][j].symbol;
			start[word_num] = _atomized_sentence[i].possibilities[this_best][j].start;
			end[word_num] = _atomized_sentence[i].possibilities[this_best][j].end;
			map[word_num] = i;
			word_num--;
		}
		this_best = _words[i][this_best].prev_poss_index;
	}
//	PrintTrellis();
	return final_size;
}


int MorphDecoder::walkForwardThroughSentence(){
	double word_prob = 0;
	double score = 0;
	double prev_score = 0;
	double scores[4];
	Symbol prev_word = model->_START;
	Symbol original_word;
	Symbol this_word;

	if (_num_tokens <= 0) {
		return 0;
		//throw UnrecoverableException("MorphDecoder::walkForwardThroughSentence()", "Empty sentence (_num_tokens=0)");
	}

	for (int this_token_index = 0; this_token_index < _num_tokens; this_token_index++) {
		int this_tok_num = _atomized_sentence[this_token_index].tok_num;
		for(size_t this_poss = 0; this_poss < _atomized_sentence[this_token_index].possibilities.size(); this_poss++) {
			//calculate the prob for the first chunk in the sequence, this can be preceded by any of
			//this_token_index - 1 words
			original_word = _atomized_sentence[this_token_index].possibilities[this_poss][0].symbol;
			this_word = original_word;

			if (!model->wordIsInVocab(this_word)) {
				this_word = model->getTrainingWordFeatures(this_word);
				if (!model->wordIsInVocab(this_word)) {
					this_word =model->getTrainingReducedWordFeatures(original_word);
				}
			}

			// Fill the word_structure array for this token/poss.
			_words[this_token_index][this_poss].word_structure.clear();
			for (size_t i = 0; i <  _atomized_sentence[this_token_index].possibilities[this_poss].size(); i++) {
                _words[this_token_index][this_poss].word_structure.push_back(
					_atomized_sentence[this_token_index].possibilities[this_poss][i].symbol);
			}

			// Add the first word to the words_used array.
			_words[this_token_index][this_poss].words_used.clear();
			_words[this_token_index][this_poss].words_used.push_back(this_word);

			_words[this_token_index][this_poss].alphaProb = model->_LOG_OF_ZERO;
			_words[this_token_index][this_poss].prev_poss_index = 0;

			//get the back pointer to the previous word
			//the back pointer is determined by only the first word of this sequence
			//and the last element of each of the previous tokens segments
			if (this_token_index == 0) {
				prev_word = model->_START;
				word_prob = model->getWordProbability(this_word, prev_word);
				score = word_prob + prev_score;
				_words[this_token_index][this_poss].alphaProb = score;
				//for printing trellis
				model->getWordProbability(this_word, prev_word, scores);
				_words[this_token_index][this_poss].scoreParts[0] = scores[0];
				_words[this_token_index][this_poss].scoreParts[1] = scores[1];
				_words[this_token_index][this_poss].scoreParts[2] = scores[2];
				_words[this_token_index][this_poss].scoreParts[3] = scores[3];
			}
			else{
				int prev_token_index = this_token_index - 1;
				int num_prev_choices = static_cast<int>(_atomized_sentence[prev_token_index].possibilities.size());
				for (int prev_poss = 0; prev_poss < num_prev_choices; prev_poss++) {
					prev_score = _words[prev_token_index][prev_poss].alphaProb;
					//look at the last word of the prev_poss word structure
					prev_word = _words[prev_token_index][prev_poss].words_used.back();
					word_prob = model->getWordProbability(this_word, prev_word);
					score = word_prob + prev_score;
					if (score > _words[this_token_index][this_poss].alphaProb) {
						_words[this_token_index][this_poss].alphaProb = score;
						_words[this_token_index][this_poss].prev_poss_index = prev_poss;
						//for printing trellis
						model->getWordProbability(this_word, prev_word, scores);
						_words[this_token_index][this_poss].scoreParts[0] = scores[0];
						_words[this_token_index][this_poss].scoreParts[1] = scores[1];
						_words[this_token_index][this_poss].scoreParts[2] = scores[2];
						_words[this_token_index][this_poss].scoreParts[3] = scores[3];
					}
				}
			}
            //calculate the probabilities for this entire path of words in this possibility
			for (size_t this_chunk = 1; this_chunk < _atomized_sentence[this_token_index].possibilities[this_poss].size(); this_chunk++) {
				prev_score = _words[this_token_index][this_poss].alphaProb;
				prev_word = _words[this_token_index][this_poss].words_used.back();
				original_word = _atomized_sentence[this_token_index].possibilities[this_poss][this_chunk].symbol;
				this_word = original_word;
				if (!model->wordIsInVocab(this_word)) {
					this_word = model->getTrainingWordFeatures(this_word);
				}
				//fill in word used so next word conditions on correct word
				assert(this_chunk == _words[this_token_index][this_poss].words_used.size());
				_words[this_token_index][this_poss].words_used.push_back(this_word);
				_words[this_token_index][this_poss].word_structure[this_chunk] = original_word;
				word_prob = model->getWordProbability(this_word, prev_word);
				_words[this_token_index][this_poss].alphaProb = word_prob + prev_score;
				//for printing trellis
				model->getWordProbability(this_word, prev_word, scores);
				_words[this_token_index][this_poss].scoreParts[0] = scores[0];
				_words[this_token_index][this_poss].scoreParts[1] = scores[1];
				_words[this_token_index][this_poss].scoreParts[2] = scores[2];
				_words[this_token_index][this_poss].scoreParts[3] = scores[3];
			}
		}
	}
	//get best sentence end
	int last_word_index = _num_tokens -1;
	int num_last_word = static_cast<int>(_atomized_sentence[last_word_index].possibilities.size());
	double best_score = model->_LOG_OF_ZERO;
	int best_index = 0;
	for (int j = 0; j < num_last_word; j++) {
		prev_score = _words[last_word_index][j].alphaProb;
		//get the last word
		prev_word = _words[last_word_index][j].words_used.back();
		word_prob = model->getWordProbability(model->_END, prev_word);
		score = word_prob + prev_score;
		if (score > best_score) {
			best_score = score;
			best_index = j;
		}
	}
	return best_index;
}


void MorphDecoder::printTrellis() {
	Symbol this_word;
	Symbol word_used;
	int back_pointer;
	double score;
	double lambda, min_lambda, bigram, unigram;
	char buffer[10000];
	char word_buffer[1000];
	char used_buffer[1000];

	if (_num_tokens <= 0) {
		return;
		//throw UnrecoverableException("MorphDecoder::walkForwardThroughSentence()", "Empty sentence (_num_tokens=0)");
	}

	for (int this_token_index = 0; this_token_index < _num_tokens; this_token_index++) {
		for (size_t this_poss = 0; this_poss < 15; this_poss++) {
			if (this_poss < _atomized_sentence[this_token_index].possibilities.size()) {
				score = _words[this_token_index][this_poss].alphaProb;
	
				bigram = _words[this_token_index][this_poss].scoreParts[0];
				lambda = _words[this_token_index][this_poss].scoreParts[1];
				unigram = _words[this_token_index][this_poss].scoreParts[2];
				min_lambda = _words[this_token_index][this_poss].scoreParts[3];

				//scoring parts
				back_pointer = 0;
				back_pointer = _words[this_token_index][this_poss].prev_poss_index;
				strcpy(word_buffer, "RealWord: ");
				size_t this_chunk;
				for (this_chunk = 0; this_chunk < _atomized_sentence[this_token_index].possibilities[this_poss].size(); this_chunk++) {
					this_word = _words[this_token_index][this_poss].word_structure[this_chunk];
					strcat(word_buffer, this_word.to_debug_string());
					strcat(word_buffer, "-");
				}
				strcpy(used_buffer, "UsedWord: ");
				for (this_chunk = 0; this_chunk < _atomized_sentence[this_token_index].possibilities[this_poss].size(); this_chunk++) {
					word_used = _words[this_token_index][this_poss].words_used[this_chunk];
					strcat(used_buffer, word_used.to_debug_string());
					strcat(used_buffer, "-");
				}

				strcpy(buffer,"");
				sprintf(buffer, " Score: %.5f (%.5f,%.5f,%.5f,%.5f) Backpointer: %.2d %25.25s %25.25s",
					score, bigram, lambda, unigram, min_lambda, back_pointer, word_buffer, used_buffer);
//				sprintf(buffer, "Token: %d Score: %d Backpointer: %d RealWord: %s WordUsed: %s",
//					this_tok_num, score, back_pointer, word_buffer, used_buffer);

				std::cout << buffer << "\t ";
				std::cout.flush();
			}
			else {
				sprintf(buffer, "%20.20s----------%20.20s"," "," ");
				strcat(buffer, "\0");
				std::cout << buffer << " ";
			}

		}
		std::cout << std::endl;
	}
}

void MorphDecoder::printTrellis(UTF8OutputStream &out) {
	Symbol this_word;
	Symbol word_used;
	int back_pointer;
	double score;
	double lambda, min_lambda, bigram, unigram;
	const int bsize = 10000;
	wchar_t buffer[bsize];
	wchar_t word_buffer[1000];
	wchar_t used_buffer[1000];

	for (int this_token_index = 0; this_token_index < _num_tokens; this_token_index++) {
		for (size_t this_poss = 0; this_poss < 15; this_poss++) {
			if (this_poss < _atomized_sentence[this_token_index].possibilities.size()) {
				score = _words[this_token_index][this_poss].alphaProb;
	
				bigram = _words[this_token_index][this_poss].scoreParts[0];
				lambda = _words[this_token_index][this_poss].scoreParts[1];
				unigram = _words[this_token_index][this_poss].scoreParts[2];
				min_lambda = _words[this_token_index][this_poss].scoreParts[3];

				//scoring parts
				back_pointer = 0;
				back_pointer = _words[this_token_index][this_poss].prev_poss_index;
				wcscpy(word_buffer, L"RealWord: ");
				size_t this_chunk;
				for (this_chunk = 0; this_chunk < _atomized_sentence[this_token_index].possibilities[this_poss].size(); this_chunk++) {
					this_word = _words[this_token_index][this_poss].word_structure[this_chunk];
					wcscat(word_buffer, this_word.to_string());
					wcscat(word_buffer, L"-");
				}
				wcscpy(used_buffer, L"UsedWord: ");
				for (this_chunk = 0; this_chunk < _atomized_sentence[this_token_index].possibilities[this_poss].size(); this_chunk++) {
					word_used = _words[this_token_index][this_poss].words_used[this_chunk];
					wcscat(used_buffer, word_used.to_string());
					wcscat(used_buffer, L"-");
				}

				wcscpy(buffer, L"");
				swprintf(buffer, bsize, L" Score: %.5f (%.5f,%.5f,%.5f,%.5f) Backpointer: %.2d %25.25ls %25.25ls",
					score, bigram, lambda, unigram, min_lambda, back_pointer, word_buffer, used_buffer);
//				swprintf(buffer, "Token: %d Score: %d Backpointer: %d RealWord: %s WordUsed: %s",
//					this_tok_num, score, back_pointer, word_buffer, used_buffer);

				out << buffer << L"\t ";
			}
			else {
				swprintf(buffer, bsize, L"%20.20ls----------%20.20ls",L" ",L" ");
				wcscat(buffer, L"\0");
				out << buffer << L" ";
			}

		}
		out << L"\n";
	}
}

Symbol MorphDecoder::makeWordFromChunks(Symbol* chunks, int num) {
	wchar_t word_buffer[MAX_LETTERS_PER_WORD];
	size_t word_len = 0;
	size_t chunk_len;
	int c = 0;
	for (int i = 0; i < num; i++) {
		const wchar_t* chunk = chunks[i].to_string();
		chunk_len = wcslen(chunk);
		word_len += chunk_len + 1;
		if (word_len > MAX_LETTERS_PER_WORD) {
			throw UnrecoverableException("MorphDecoder::makeWordFromChunks()",
										 "Possible word too long");
		}
		if (i < num - 1)
			wcscat(word_buffer, L"+");
		wcscat(word_buffer, chunk);
	}
	return Symbol(word_buffer);
}


boost::shared_ptr<MorphDecoder::Factory> &MorphDecoder::_factory() {
	static boost::shared_ptr<MorphDecoder::Factory> factory(new GenericMorphDecoderFactory());
	return factory;
}

