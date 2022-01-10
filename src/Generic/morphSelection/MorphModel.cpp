// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/morphSelection/MorphModel.h"
#include "Generic/morphSelection/xx_MorphModel.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/theories/Token.h"
#include <math.h>
#include <boost/scoped_ptr.hpp>

const Symbol MorphModel::_START = Symbol(L":START");
const Symbol MorphModel::_END = Symbol(L":END");
const double MorphModel::_LOG_OF_ZERO = -1000;

MorphModel::MorphModel() {

	_unknown_threshold = 3;
	_mult = 4;
	_unigramProbTable = _new NgramScoreTable(1, 15000);
	_bigramProbTable = _new NgramScoreTable(2, 15000);
	_uniqueHistoryCount =  _new NgramScoreTable(1, 15000);
	_lambdaTable = _new NgramScoreTable(1, 15000);
	_knownVocabTable = _new NgramScoreTable(1, 15000);
	_vocabTable = _new NgramScoreTable(1, 15000);
	_featureTable = _new NgramScoreTable(1, 500);	//count the number of unique words that have feature x
	_unigramCount = _new NgramScoreTable(1, 20000);
	_bigramCount = _new NgramScoreTable(2, 15000);
	_featureWordPairs = _new NgramScoreTable(2, 500);
	_uniqFeatureCounts = _new NgramScoreTable(1, 500);
	_featureCounts = _new NgramScoreTable(1, 500);

	_bigramFeatureWordPairs = _new NgramScoreTable(4, 500);
	_bigramUniqFeatureCounts = _new NgramScoreTable(2, 500);
	_bigramFeatureCounts = _new NgramScoreTable(2, 500);

}

MorphModel::MorphModel(const char* model_file_prefix) {
	_unknown_threshold = 3;
	_mult = 4;
	
	char buffer[500];
	boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& inStream(*inStream_scoped_ptr);
	sprintf(buffer,"%s.unigramprob", model_file_prefix);
	inStream.open(buffer);
	_unigramProbTable = _new NgramScoreTable(1, inStream);
	inStream.close();

	sprintf(buffer,"%s.bigramprob", model_file_prefix);
	inStream.open(buffer);
	_bigramProbTable = _new NgramScoreTable(2, inStream);
	inStream.close();
	
	sprintf(buffer,"%s.lambda", model_file_prefix);
	inStream.open(buffer);
	_lambdaTable = _new NgramScoreTable(1, inStream);
	inStream.close();

	_vocabTable = _new NgramScoreTable(1, _unigramProbTable->get_size());
	NgramScoreTable::Table::iterator iter;
	for(iter = _unigramProbTable->get_start(); iter != _unigramProbTable->get_end(); ++iter){
		_vocabTable->add((*iter).first,1);
	}

	_uniqueHistoryCount = 0;
	_knownVocabTable = 0;
	_featureTable = 0;	//count the number of unique words that have feature x
	_unigramCount = 0;
	_bigramCount = 0;
	_featureWordPairs = 0;
	_uniqFeatureCounts = 0;
	_featureCounts = 0;
	_bigramFeatureWordPairs = 0;
	_bigramUniqFeatureCounts = 0;
	_bigramFeatureCounts = 0;
}

MorphModel::~MorphModel() {
	delete _unigramProbTable;
	delete _bigramProbTable;
	delete _uniqueHistoryCount;
	delete _lambdaTable;
	delete _knownVocabTable;
	delete _vocabTable;
	delete _featureTable;
	delete _unigramCount;
	delete _bigramCount;
	delete _featureWordPairs;
	delete _uniqFeatureCounts;
	delete _featureCounts;
	delete _bigramFeatureWordPairs;
	delete _bigramUniqFeatureCounts;
	delete _bigramFeatureCounts;
}

void MorphModel::trainOnSingleFile(const char* train_file, const char* model_prefix) {
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);

	// collect vocabulary
	uis.open(train_file);
	int size = getVocab(uis);
	uis.close();
	
	// collect counts
	uis.open(train_file);
	collectCounts(uis);
	uis.close();
	
	// derive and print model
	deriveProb();
	deriveLambda();
	printTables(model_prefix);
}


double MorphModel::getWordProbability(Symbol word, Symbol prev_word) {
	Symbol bigram[2];
	Symbol unigram[1];
	unigram[0] = word;
	bigram[1] = prev_word;
	bigram[0] = word;
	
	float bigram_prob = _bigramProbTable->lookup(bigram);
	float unigram_prob = _unigramProbTable->lookup(unigram);
	float lambda = _lambdaTable->lookup(unigram);
	float prob = lambda * bigram_prob + (1-lambda)*unigram_prob;
	
	if (prob == 0)
		return _LOG_OF_ZERO;
	else
		return log(prob);
}

double MorphModel::getWordProbability(Symbol word, Symbol prev_word, double scores[]) {
	Symbol bigram[2];
	Symbol unigram[1];
	unigram[0]= word;
	bigram[1] = prev_word;
	bigram[0] = word;
	
	float bigram_prob = _bigramProbTable->lookup(bigram);
	float unigram_prob = _unigramProbTable->lookup(unigram);
	float lambda = _lambdaTable->lookup(unigram);
	float prob = lambda * bigram_prob + (1-lambda)*unigram_prob;

	scores[0] = bigram_prob;
	scores[1] = lambda;
	scores[2] = unigram_prob;
	scores[3] = 1-lambda;

	if (prob == 0)
		return _LOG_OF_ZERO;
	else
		return log(prob);
}

int MorphModel::getVocab(UTF8InputStream& uis) {
	Symbol sentence[MAX_SENTENCE_TOKENS];
	Symbol ngram[1];
	int s = 0;
	while (!uis.eof()) {
		s++;
        int sent_length = readSentence(uis, sentence, MAX_SENTENCE_TOKENS);
		for(int i = 0; i < sent_length; i++){
			ngram[0] = sentence[i];
			_vocabTable->add(ngram, 1);
		}
	}
	//prune
	NgramScoreTable::Table::iterator start = _vocabTable->get_start();
	NgramScoreTable::Table::iterator end = _vocabTable->get_end();	
	NgramScoreTable::Table::iterator iter;
	for (iter = start; iter != end; ++iter) {
		if ((*iter).second > _unknown_threshold) {
			_knownVocabTable->add((*iter).first);
		}
		else {
			Symbol feat = getTrainingWordFeatures((*iter).first[0]);
			_featureTable->add(&feat);
		}
	}
	return _knownVocabTable->get_size();
}

void MorphModel::collectCounts(UTF8InputStream& uis) {
	Symbol sentence[MAX_SENTENCE_TOKENS];
	Symbol ngram[2];
	Symbol featureNgram[2];
	Symbol wordFeatureBigram[4];

	Symbol prev_word;
	Symbol curr_features;
	bool prevUnknown;
	bool currUnknown;
	int i;
	
	while (!uis.eof()) {
        
		int sent_length = readSentence(uis, sentence, MAX_SENTENCE_TOKENS);
		
		prev_word = _START;
		_unigramCount->add(&prev_word);
		prevUnknown = false;
		currUnknown = false;
		
		for (i = 0; i < sent_length; i++) {
			currUnknown = false;
			ngram[0] = sentence[i];
			ngram[1] = prev_word;

			if(!_knownVocabTable->lookup(&sentence[i])){
				curr_features =  getTrainingWordFeatures(sentence[i]);
				currUnknown = true;
			}

			//Add bigram info for unknown words
			if (prevUnknown || currUnknown) {

				featureNgram[0] = prev_word;
				featureNgram[1] = sentence[i];

				wordFeatureBigram[0] = prev_word;
				wordFeatureBigram[1] = (i > 0) ? sentence[i-1] : prev_word;
				wordFeatureBigram[2] = sentence[i];
				wordFeatureBigram[3] = sentence[i];
				
				if (currUnknown) {
					featureNgram[1] = curr_features;
					wordFeatureBigram[2] = curr_features;
				}

				if (_bigramFeatureWordPairs->lookup(wordFeatureBigram) <= 0) {
					_bigramUniqFeatureCounts->add(featureNgram, 1);
				}
				_bigramFeatureWordPairs->add(wordFeatureBigram, 1);
				_bigramFeatureCounts->add(featureNgram, 1);
				prevUnknown = false;
			}

			//Add unigram info for unknown words
			if (currUnknown) {
				featureNgram[0] = curr_features;
				featureNgram[1] = sentence[i];
				if (_featureWordPairs->lookup(featureNgram) <= 0) {
					_uniqFeatureCounts->add(&featureNgram[0]);
				}
				_featureCounts->add(&featureNgram[0]);
				_featureWordPairs->add(featureNgram, 1);
				//also set ngram[0] to feature
				ngram[0] = curr_features;
				prevUnknown = true;
			}		
			_unigramCount->add(&ngram[0]);
			_bigramCount->add(ngram);
			prev_word = ngram[0];
		}

		ngram[1] = prev_word;
		ngram[0] = _END;
		_bigramCount->add(ngram);
		_unigramCount->add(&ngram[0]);

		//add unknown words for end case
		if (prevUnknown) {
			featureNgram[0] = prev_word;
			featureNgram[1] = _END;

			wordFeatureBigram[0] = prev_word;
			wordFeatureBigram[1] = (i > 0) ? sentence[i-1] : prev_word;
			wordFeatureBigram[2] = _END;
			wordFeatureBigram[3] = _END;

			if(_bigramFeatureWordPairs->lookup(wordFeatureBigram) <= 0) {
				_bigramUniqFeatureCounts->add(featureNgram, 1);
			}
			_bigramFeatureWordPairs->add(wordFeatureBigram, 1);
			_bigramFeatureCounts->add(featureNgram, 1);
		}


		//loop through the sentence a second time, this time
		//just add reduced features
		prev_word = _START;
		bool prev_was_unknown = false;
		prevUnknown = false;
		currUnknown = false;

		for (i = 0; i < sent_length; i++) {
			ngram[0] = sentence[i];
			ngram[1] = prev_word;
			if(!_knownVocabTable->lookup(&sentence[i])){
				curr_features =  getTrainingReducedWordFeatures(sentence[i]);
				currUnknown = true;
				ngram[0] = curr_features;
			}
			if (prevUnknown || currUnknown) {

				featureNgram[0] = prev_word;
				featureNgram[1] = sentence[i];

				wordFeatureBigram[0] = prev_word;
				wordFeatureBigram[1] = (i > 0) ? sentence[i-1] : prev_word;				
				wordFeatureBigram[2] = sentence[i];
				wordFeatureBigram[3] = sentence[i];

				if (currUnknown) {
					featureNgram[1] = curr_features;
					wordFeatureBigram[2] = curr_features;
				}

				if(_bigramFeatureWordPairs->lookup(wordFeatureBigram) <= 0) {
					_bigramUniqFeatureCounts->add(featureNgram, 1);
				}
				_bigramFeatureWordPairs->add(wordFeatureBigram, 1);
				_bigramFeatureCounts->add(featureNgram, 1);
				
				prevUnknown = false;
			}

			//Add unigram info for unknown words
			if (currUnknown) {
				featureNgram[0] = curr_features;
				featureNgram[1] = sentence[i];
				if(_featureWordPairs->lookup(featureNgram) <= 0) {
					_uniqFeatureCounts->add(&featureNgram[0]);
				}
				_featureCounts->add(&featureNgram[0]);
				_featureWordPairs->add(featureNgram, 1);
				//also set ngram[0] to feature
				_unigramCount->add(&ngram[0]);
				_bigramCount->add(ngram);
				prevUnknown = true;
				prev_was_unknown = true;
			}
			else if (prev_was_unknown) {
				_bigramCount->add(ngram);
				prev_was_unknown = false;
			}
			prev_word = ngram[0];
		}

		if (prev_was_unknown) {
			ngram[1] = prev_word;
			ngram[0] = _END;
			_bigramCount->add(ngram);

			featureNgram[0] = prev_word;
			featureNgram[1] = _END;

			wordFeatureBigram[0] = prev_word;
			wordFeatureBigram[1] = (i > 0) ? sentence[i-1] : prev_word;
			wordFeatureBigram[2] = _END;
			wordFeatureBigram[3] = _END;

			if(_bigramFeatureWordPairs->lookup(wordFeatureBigram) <= 0) {
				_bigramUniqFeatureCounts->add(featureNgram, 1);
			}
			_bigramFeatureWordPairs->add(wordFeatureBigram, 1);
			_bigramFeatureCounts->add(featureNgram, 1);
		}
	}
}

void MorphModel::deriveProb() {
	NgramScoreTable::Table::iterator iter;
	float size = 0;
	float transitions;
	float histories;
	float n;
	Symbol hist_word;
	Symbol word;

	for (iter = _unigramCount->get_start(); iter != _unigramCount->get_end(); ++iter) {
		word = (*iter).first[0];
		//modify counts of features:  #occ/#uniq_occ
		//this is the average number of times any single word in the class defined by 
		//the feature occurs.  
		if (_uniqFeatureCounts->lookup(&word) > 0) {
			n = (_featureCounts->lookup(&word)/ _uniqFeatureCounts->lookup(&word));
			size += n;
			(*iter).second = n;
		}
		else {
			size += (*iter).second;
		}
	}
	_numberOfTrainingWords = size;
	for (iter = _unigramCount->get_start(); iter != _unigramCount->get_end(); ++iter) {
		_unigramProbTable->add((*iter).first, (*iter).second/_numberOfTrainingWords);
	}
	
	for (iter = _bigramCount->get_start(); iter != _bigramCount->get_end(); ++iter) {
		//modify the counts of bigrams that include features in the same way as above
		if (_bigramFeatureCounts->lookup((*iter).first) > 0) {
			transitions = _bigramFeatureCounts->lookup((*iter).first)/
						  _bigramUniqFeatureCounts->lookup((*iter).first);
		}
		else {
			transitions = (*iter).second;
		}
		hist_word = (*iter).first[1];
		histories = _unigramCount->lookup(&hist_word);
		_bigramProbTable->add((*iter).first, transitions/histories);
		_uniqueHistoryCount->add(&hist_word);
	}	
}

void MorphModel::deriveLambda() {
	Symbol word;
	NgramScoreTable::Table::iterator iter;
	for (iter = _unigramCount->get_start(); iter != _unigramCount->get_end(); ++iter) {
		word = (*iter).first[0];
		float hist = (*iter).second;
		float uniq =  _uniqueHistoryCount->lookup(&word);
		_lambdaTable->add((*iter).first, (hist /(hist + (_mult* uniq))));
	}
}

void MorphModel::printTables(const char* model_file_prefix) {
	UTF8OutputStream outStream;
	char buffer[500];
	std::cout << "Printing tables, prefix: " << model_file_prefix << std::endl;
	sprintf(buffer,"%s.unigramcounts", model_file_prefix);
	outStream.open(buffer);
	_unigramCount->print_to_open_stream(outStream);
	outStream.close();

	sprintf(buffer,"%s.bigramcounts", model_file_prefix);
	outStream.open(buffer);
	_bigramCount->print_to_open_stream(outStream);
	outStream.close();

	sprintf(buffer,"%s.unigramprob", model_file_prefix);
	outStream.open(buffer);
	_unigramProbTable->print_to_open_stream(outStream);
	outStream.close();

	sprintf(buffer,"%s.bigramprob", model_file_prefix);
	outStream.open(buffer);
	_bigramProbTable->print_to_open_stream(outStream);
	outStream.close();

	sprintf(buffer,"%s.lambda", model_file_prefix);
	outStream.open(buffer);
	_lambdaTable->print_to_open_stream(outStream);
	outStream.close();

	sprintf(buffer,"%s.featureTable", model_file_prefix);
	outStream.open(buffer);
	_featureTable->print_to_open_stream(outStream);
	outStream.close();
}

/* Training sentences have the same format as standalone idf decode sentences-
( w1 w2 w3 ....... wn )
*/

int MorphModel::readSentence(UTF8InputStream& stream, Symbol* sentence, int num_words) {
	UTF8Token token;
	
	if (stream.eof())
		return 0;
	stream >> token;
	if (stream.eof())
		return 0;

	if (token.symValue() != SymbolConstants::leftParen)
		throw UnexpectedInputException("MorphModel::readSentence", 
										"Ill-formed sentence");

	int length = 0;
	while (true) {
		stream >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;	
		sentence[length] = token.symValue();
		length++;
		if(length > num_words){
			//truncate sentence
			break;
		}
	}
	return length;
	
}

bool MorphModel::containsDigit(Symbol wordSym) const {
	std::wstring word = wordSym.to_string();
	size_t length = word.length();
	bool contains_digit = false;
	for (size_t i = 0; i < length; ++i) {
		if (iswdigit(word[i])){
			contains_digit = true;
			break;
		}
	}
	return contains_digit;
}

bool MorphModel::containsASCII(Symbol wordSym) const {
	std::wstring word = wordSym.to_string();
	size_t length = word.length();
	for(size_t i = 0; i<  length; ++i){
		if(iswascii(word[i])){
			return true;
		}
	}
	return false;
}


boost::shared_ptr<MorphModel::Factory> &MorphModel::_factory() {
	static boost::shared_ptr<MorphModel::Factory> factory(new GenericMorphModelFactory());
	return factory;
}

