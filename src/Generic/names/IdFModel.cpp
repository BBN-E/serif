// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFModel.h"
#include "Generic/names/IdFSentence.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"

#include <math.h>
#include <boost/scoped_ptr.hpp>

#define OUTPUT_ALL_PROBS false

Symbol IdFModel::NOT_SEEN = Symbol(L":NOT_SEEN");

/**
 * Initializes IdFModel for training. 
 */
IdFModel::IdFModel(const NameClassTags *nameClassTags, 
				   const IdFWordFeatures *wordFeatures) :
	LOG_OF_ZERO(-10000), DECODE(false), _nameClassTags(nameClassTags),
	TAG_PROBS_HAVE_BEEN_PRE_COMPUTED(false), WORD_PROBS_HAVE_BEEN_PRE_COMPUTED(false),
	_wordFeatures(wordFeatures)
{	
	//CHINESE... currently set in IdFTrainerMain...
	//setKValues(4, 5, 3, 9, 9);
	
	//ENGLISH/ARABIC: 
	setKValues(2, 4, 11, 3, 3);
	
	// w | T, T-1
	_wordStartProbabilities = _new NgramScoreTable(3, 10000);
	_wordStartLambdas = _new NgramScoreTable(2, 10000);

	// w | T, w-1
	_wordContinueProbabilities = _new NgramScoreTable(3, 10000);
	_wordContinueLambdas = _new NgramScoreTable(2, 10000);

	// w | T
	_wordTagProbabilities = _new NgramScoreTable(2, 10000);
	_wordTagLambdas = _new NgramScoreTable(1, 10000);

	// w | reduced-T
	_wordReducedTagProbabilities = _new NgramScoreTable(2, 10000);
	_wordReducedTagLambdas = _new NgramScoreTable(1, 10000);

	// T | T-1, w-1
	_tagFullProbabilities = _new NgramScoreTable(3, 10000);
	_tagFullLambdas = _new NgramScoreTable(2, 10000);

	// T | T-1
	_tagReducedProbabilities = _new NgramScoreTable(2, 10000);
	
	// vocabulary
	_vocabularyTable = _new NgramScoreTable(1, 10000);

	// not to be printed out, just for training
	_startUniqueCounts = _new NgramScoreTable(2, 10000);
	_continueUniqueCounts = _new NgramScoreTable(2, 10000);
	_wordTagUniqueCounts = _new NgramScoreTable(1, 10000);
	_wordReducedTagUniqueCounts = _new NgramScoreTable(1, 10000);
	_wordStartHistories = _new NgramScoreTable(2, 10000);
	_wordContinueHistories = _new NgramScoreTable(2, 10000);
	_wordTagHistories = _new NgramScoreTable(1, 10000);
	_wordReducedTagHistories = _new NgramScoreTable(1, 10000);

	_tagUniqueCounts = _new NgramScoreTable(2, 10000);
	_tagFullHistories = _new NgramScoreTable(2, 10000);
	_tagReducedHistories = _new NgramScoreTable(1, 10000);
	_tagPriors = _new NgramScoreTable(1, 1000);

	// will be created later
	_preComputedFirstLevelLambdas = 0;
	_preComputedSecondLevelLambdas = 0;
	_preComputedThirdLevelLambdas = 0;
}

/**
 * Initializes IdFModel for decode. 
 */
IdFModel::IdFModel(const NameClassTags *nameClassTags, const char *model_file_prefix,
				   const IdFWordFeatures *wordFeatures) :
	LOG_OF_ZERO(-10000), DECODE(true), _nameClassTags(nameClassTags),
	TAG_PROBS_HAVE_BEEN_PRE_COMPUTED(true), WORD_PROBS_HAVE_BEEN_PRE_COMPUTED(true),
	_wordFeatures(wordFeatures)
{	

	boost::scoped_ptr<UTF8InputStream> tagStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& tagStream(*tagStream_scoped_ptr);
	char buffer[500];
	sprintf(buffer,"%s.tagprobs", model_file_prefix);
//	verifyEntityTypes(buffer); // SRS -- turned off b'c it caused problems
	tagStream.open(buffer);

	_tagFullLambdas = _new NgramScoreTable(2, tagStream);
	_tagFullProbabilities = _new NgramScoreTable(3, tagStream);
	_tagReducedProbabilities = _new NgramScoreTable(2, tagStream);

	tagStream.close();

	boost::scoped_ptr<UTF8InputStream> wordStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& wordStream(*wordStream_scoped_ptr);
	sprintf(buffer,"%s.wordlambdas", model_file_prefix);
	wordStream.open(buffer);

	_preComputedFirstLevelLambdas = _new NgramScoreTable(2, wordStream);
	_preComputedSecondLevelLambdas = _new NgramScoreTable(2, wordStream);
	_preComputedThirdLevelLambdas = _new NgramScoreTable(2, wordStream);

	wordStream.close();

	sprintf(buffer,"%s.wordtagprobs", model_file_prefix);
	wordStream.open(buffer);

	_wordTagProbabilities = _new NgramScoreTable(2, wordStream);
	_wordReducedTagProbabilities = _new NgramScoreTable(2, wordStream);

	wordStream.close();

	sprintf(buffer,"%s.wordprobs", model_file_prefix);
	wordStream.open(buffer);

	_wordStartProbabilities = _new NgramScoreTable(3, wordStream);
	_wordContinueProbabilities = _new NgramScoreTable(3, wordStream);

	wordStream.close();
	
	boost::scoped_ptr<UTF8InputStream> vocabStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& vocabStream(*vocabStream_scoped_ptr);
	sprintf(buffer,"%s.vocab", model_file_prefix);
	vocabStream.open(buffer);

	_vocabularyTable = _new NgramScoreTable(1, vocabStream);
	vocabStream.close();
	calculateOneOverVocabSize();
	
}

/** prints tables to:
 *  model_file_prefix.tagprobs
 *  model_file_prefix.wordlambdas
 *  model_file_prefix.wordtagprobs
 *  model_file_prefix.wordprobs
 *  model_file_prefix.vocab
 */
void IdFModel::printTables(const char *model_file_prefix) {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't print out a model during decode\n";
		return;
	}
	
	preComputeWordProbs();
	preComputeTagProbs();

	UTF8OutputStream outStream;
	char buffer[500];
	sprintf(buffer,"%s.tagprobs", model_file_prefix);
	outStream.open(buffer);

	_tagFullLambdas->print_to_open_stream(outStream);
	_tagFullProbabilities->print_to_open_stream(outStream);
	_tagReducedProbabilities->print_to_open_stream(outStream);

	outStream.close();

	sprintf(buffer,"%s.wordlambdas", model_file_prefix);
	outStream.open(buffer);

	_preComputedFirstLevelLambdas->print_to_open_stream(outStream);
	_preComputedSecondLevelLambdas->print_to_open_stream(outStream);
	_preComputedThirdLevelLambdas->print_to_open_stream(outStream);

	outStream.close();
	
	sprintf(buffer,"%s.wordtagprobs", model_file_prefix);
	outStream.open(buffer);

	_wordTagProbabilities->print_to_open_stream(outStream);
	_wordReducedTagProbabilities->print_to_open_stream(outStream);

	outStream.close();

	sprintf(buffer,"%s.wordprobs", model_file_prefix);
	outStream.open(buffer);

	_wordStartProbabilities->print_to_open_stream(outStream);
	_wordContinueProbabilities->print_to_open_stream(outStream);

	outStream.close();

	sprintf(buffer,"%s.vocab", model_file_prefix);
	outStream.open(buffer);

	_vocabularyTable->print_to_open_stream(outStream);

	outStream.close();

}

void IdFModel::calculateOneOverVocabSize() {
	NgramScoreTable::Table::iterator iter;
	float vocab_size = 0;
	for (iter = _vocabularyTable->get_start(); iter != _vocabularyTable->get_end(); ++iter) {
		vocab_size += (*iter).second;
	}
	//_oneOverVocabSize = 1 / vocab_size;
	_oneOverVocabSize = 1.0f / static_cast<float>(_vocabularyTable->get_size());
}

/**
 * Given dataTable (a set of events), fill in probability tables (future + history, 
 * # times occured), history tables (history, # times occured), and unique tables 
 * (history, # unique futures given this history). 
 *
 * Calculate the un-backed-off probabilities in the prob tables, then call 
 * deriveWordLambdas() to fill in the lambda tables, using the counts from 
 * the history and unique tables.
 */
void IdFModel::deriveWordTables(NgramScoreTable* dataTable) {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't derive word tables during decode\n";
		return;
	}

	NgramScoreTable::Table::iterator iter;
	for (iter = dataTable->get_start(); iter != dataTable->get_end(); ++iter) {

		Symbol ngram[3];

		ngram[0] = (*iter).first[0]; // word
		ngram[1] = (*iter).first[1]; // full tag
		_wordTagProbabilities->add(ngram, (*iter).second);
		_wordTagHistories->add(ngram + 1, (*iter).second);

		ngram[1] = (*iter).first[2]; // reduced tag
		_wordReducedTagProbabilities->add(ngram, (*iter).second);
		_wordReducedTagHistories->add(ngram + 1, (*iter).second);

		if (_nameClassTags->isContinue((*iter).first[3])) {
			ngram[0] = (*iter).first[0]; // word
			ngram[1] = (*iter).first[1]; // tag
			ngram[2] = (*iter).first[5]; // word - 1
			_wordContinueProbabilities->add(ngram, (*iter).second);
			_wordContinueHistories->add(ngram + 1, (*iter).second);
		}
		// mid-start tags, not typically classified as "start") are treated as
		// start for probability model purposes
		else {		
			ngram[0] = (*iter).first[0]; // word
			ngram[1] = (*iter).first[1]; // tag
			ngram[2] = (*iter).first[4]; // tag - 1
			_wordStartProbabilities->add(ngram, (*iter).second);
			_wordStartHistories->add(ngram + 1, (*iter).second);
		}
	}

	for (iter = _wordStartProbabilities->get_start(); iter != _wordStartProbabilities->get_end(); ++iter) {
		_startUniqueCounts->add((*iter).first + 1, 1);
		float transitions = (*iter).second;
		float histories = _wordStartHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	for (iter = _wordContinueProbabilities->get_start(); iter != _wordContinueProbabilities->get_end(); ++iter) {
		_continueUniqueCounts->add((*iter).first + 1, 1);
		float transitions = (*iter).second;
		float histories = _wordContinueHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	for (iter = _wordTagProbabilities->get_start(); iter != _wordTagProbabilities->get_end(); ++iter) {
		_wordTagUniqueCounts->add((*iter).first + 1, 1);
		float transitions = (*iter).second;
		float histories = _wordTagHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	for (iter = _wordReducedTagProbabilities->get_start(); iter != _wordReducedTagProbabilities->get_end(); ++iter) {
		_wordReducedTagUniqueCounts->add((*iter).first + 1, 1);
		float transitions = (*iter).second;
		float histories = _wordReducedTagHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	deriveWordLambdas();
	
}

/**
 *  Assumes deriveWordTables has been called already. Given the counts contained
 *  in the tables filled in by deriveWordTables, it fills in the
 *  word lambda tables.
 *
 *  Can be called as many times as needed (for instance, when we change the values
 *  of the multipliers in estimating the smoothing parameters) -- it resets the 
 *  tables at the beginning of each call. 
 */
void IdFModel::deriveWordLambdas() {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't derive word lambdas during decode\n";
		return;
	}

	_wordStartLambdas->reset();
	_wordContinueLambdas->reset();
	_wordTagLambdas->reset();
	_wordReducedTagLambdas->reset();
	NgramScoreTable::Table::iterator iter;
	for (iter = _wordStartHistories->get_start(); iter != _wordStartHistories->get_end(); ++iter) {
		float histories = (*iter).second;
		float uniques = _startUniqueCounts->lookup((*iter).first);
		_wordStartLambdas->add((*iter).first, 
			histories / (histories + _wordStartWBMultiplier * uniques));
	}

	for (iter = _wordContinueHistories->get_start(); iter != _wordContinueHistories->get_end(); ++iter) {
		float histories = (*iter).second;
		float uniques = _continueUniqueCounts->lookup((*iter).first);
		_wordContinueLambdas->add((*iter).first, 
			histories / (histories + _wordContinueWBMultiplier * uniques));
	}

	for (iter = _wordTagHistories->get_start(); iter != _wordTagHistories->get_end(); ++iter) {
		float histories = (*iter).second;
		float uniques = _wordTagUniqueCounts->lookup((*iter).first);
		_wordTagLambdas->add((*iter).first, 
			histories / (histories + _wordTagWBMultiplier * uniques));
	}

	for (iter = _wordReducedTagHistories->get_start(); iter != _wordReducedTagHistories->get_end(); ++iter) {
		float histories = (*iter).second;
		float uniques = _wordReducedTagUniqueCounts->lookup((*iter).first);
		_wordReducedTagLambdas->add((*iter).first, 
			histories / (histories + _wordReducedTagWBMultiplier * uniques));
	}

}

/**
 * Assumes deriveWordTables() has been called and does some pre-computation of 
 * backed-off probabilities to speed-up decode. Modifies the tables, so this can
 * only be done once (enforced by WORD_PROBS_HAVE_BEEN_PRE_COMPUTED).
 */
void IdFModel::preComputeWordProbs() {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't calculate back-offs during decode\n";
		return;
	} else if (WORD_PROBS_HAVE_BEEN_PRE_COMPUTED) {
		SessionLogger::info("SERIF") << "word probs have already been pre-computed; can't do it twice\n";
		return;
	}

	NgramScoreTable::Table::iterator iter;
	for (iter = _wordReducedTagProbabilities->get_start(); iter != _wordReducedTagProbabilities->get_end(); ++iter) {
		float lambda = _wordReducedTagLambdas->lookup((*iter).first + 1);
		float prob = (*iter).second * lambda + (1 - lambda) * _oneOverVocabSize;
		(*iter).second = prob;
	}		

	Symbol reduced_tag_ngram[2];
	for (iter = _wordTagProbabilities->get_start(); iter != _wordTagProbabilities->get_end(); ++iter) {
		float lambda = _wordTagLambdas->lookup((*iter).first + 1);
		reduced_tag_ngram[0] = (*iter).first[0];
		reduced_tag_ngram[1] 
			= _nameClassTags->getReducedTagSymbol(_nameClassTags->getIndexForTag((*iter).first[1]));
		float prob  = (*iter).second * lambda 
			+ (1 - lambda) * _wordReducedTagProbabilities->lookup(reduced_tag_ngram);
		(*iter).second = prob;
	}

	for (iter = _wordStartProbabilities->get_start(); iter != _wordStartProbabilities->get_end(); ++iter) {
		float lambda = _wordStartLambdas->lookup((*iter).first + 1);
		float prob  = (*iter).second * lambda 
			+ (1 - lambda) * _wordTagProbabilities->lookup((*iter).first);
		(*iter).second = prob;
	}

	for (iter = _wordContinueProbabilities->get_start(); iter != _wordContinueProbabilities->get_end(); ++iter) {
		float lambda = _wordContinueLambdas->lookup((*iter).first + 1);
		float prob  = (*iter).second * lambda 
			+ (1 - lambda) * _wordTagProbabilities->lookup((*iter).first);
		(*iter).second = prob;
	}

	_preComputedFirstLevelLambdas = _new NgramScoreTable(2, 
		_wordContinueLambdas->get_size() + _wordStartLambdas->get_size());
	for (iter = _wordContinueLambdas->get_start(); iter != _wordContinueLambdas->get_end(); ++iter) {
		_preComputedFirstLevelLambdas->add((*iter).first, 1 - (*iter).second);
	}
	for (iter = _wordStartLambdas->get_start(); iter != _wordStartLambdas->get_end(); ++iter) {
		_preComputedFirstLevelLambdas->add((*iter).first, 1 - (*iter).second);
	}
	_preComputedSecondLevelLambdas = _new NgramScoreTable(2, _preComputedFirstLevelLambdas->get_size());
	for (iter = _preComputedFirstLevelLambdas->get_start(); iter != _preComputedFirstLevelLambdas->get_end(); ++iter) {
		float lambda2 = 1 - _wordTagLambdas->lookup((*iter).first);
		_preComputedSecondLevelLambdas->add((*iter).first, lambda2 * (*iter).second);
	}
	Symbol ngram[2];
	for (iter = _wordTagLambdas->get_start(); iter != _wordTagLambdas->get_end(); ++iter) {
		ngram[0] = (*iter).first[0];
		ngram[1] = NOT_SEEN;
		_preComputedSecondLevelLambdas->add(ngram, 1 - (*iter).second);
	}
	
	_preComputedThirdLevelLambdas = _new NgramScoreTable(2, _preComputedSecondLevelLambdas->get_size());
	for (iter = _preComputedSecondLevelLambdas->get_start(); iter != _preComputedSecondLevelLambdas->get_end(); ++iter) {
		Symbol reduced_tag 
			= _nameClassTags->getReducedTagSymbol(_nameClassTags->getIndexForTag((*iter).first[0]));
		float lambda3 = 1 - _wordReducedTagLambdas->lookup(&reduced_tag);
		_preComputedThirdLevelLambdas->add((*iter).first, lambda3 * (*iter).second);
	}

	WORD_PROBS_HAVE_BEEN_PRE_COMPUTED = true;
	
}

/** 
 * @return P(word | tag, tag-1, word-1)
 */
double IdFModel::getWordProbability(Symbol word, int tag, 
									int tag_minus_one, Symbol word_minus_one) 
{
	Symbol ngram[3];
	ngram[0] = word;
	ngram[1] = _nameClassTags->getTagSymbol(tag);

	double probability = 0;
	double lambda = 1;
	if (_nameClassTags->isContinue(tag)) {
		ngram[2] = word_minus_one;
		probability = _wordContinueProbabilities->lookup(ngram);
	} else {
		ngram[2] = _nameClassTags->getTagSymbol(tag_minus_one);
		probability = _wordStartProbabilities->lookup(ngram);
	}

	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << ngram[0].to_debug_string() << " "
			<< ngram[1].to_debug_string() << " "
			<< ngram[2].to_debug_string() << " ";
	}

	if (probability == 0) {
		probability = _wordTagProbabilities->lookup(ngram);
		if (probability != 0) {
			lambda = _preComputedFirstLevelLambdas->lookup(ngram + 1);
			if (lambda == 0)
				lambda = 1;
		} else {
			ngram[1] = _nameClassTags->getReducedTagSymbol(tag);
			probability = _wordReducedTagProbabilities->lookup(ngram);
			ngram[1] = _nameClassTags->getTagSymbol(tag);
			if (probability != 0) {
				lambda = _preComputedSecondLevelLambdas->lookup(ngram + 1);
				if (lambda == 0) {
					ngram[2] = NOT_SEEN;
					lambda = _preComputedSecondLevelLambdas->lookup(ngram + 1);
				}
			} else {
				probability = _oneOverVocabSize;
				lambda =  _preComputedThirdLevelLambdas->lookup(ngram + 1);
				if (lambda == 0) {
					ngram[2] = NOT_SEEN;
					lambda = _preComputedThirdLevelLambdas->lookup(ngram + 1);
				}
			}
		}
	}

	probability *= lambda;

	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << " -- " << log(probability) << "\n";
	}
	if (probability != 0)
		return log (probability);
	else return LOG_OF_ZERO;

}

/**
 * Given dataTable (a set of events), fill in probability tables (future + history, 
 * # times occured), history tables (history, # times occured), and unique tables 
 * (history, # unique futures given this history). 
 *
 * Calculate the un-backed-off probabilities in the prob tables, then call 
 * deriveTagLambdas() to fill in the lambda tables, using the counts from 
 * the history and unique tables.
 */
void IdFModel::deriveTagTables(NgramScoreTable* dataTable) {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't derive tag tables during decode\n";
		return;
	}

	NgramScoreTable::Table::iterator iter;
	float total = 0;
	for (iter = dataTable->get_start(); iter != dataTable->get_end(); ++iter) {
		_tagFullProbabilities->add((*iter).first, (*iter).second);
		_tagFullHistories->add((*iter).first + 1, (*iter).second);
		_tagUniqueCounts->add((*iter).first + 1, 1);
		_tagReducedProbabilities->add((*iter).first, (*iter).second);
		_tagReducedHistories->add((*iter).first + 1, (*iter).second);
		_tagPriors->add((*iter).first, (*iter).second);
		total += (*iter).second;
	}

	for (iter = _tagFullProbabilities->get_start(); iter != _tagFullProbabilities->get_end(); ++iter) {
		float transitions = (*iter).second;
		float histories = _tagFullHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	for (iter = _tagReducedProbabilities->get_start(); iter != _tagReducedProbabilities->get_end(); ++iter) {
		float transitions = (*iter).second;
		float histories = _tagReducedHistories->lookup((*iter).first + 1);
		(*iter).second = transitions / histories;
	}

	for (iter = _tagPriors->get_start(); iter != _tagPriors->get_end(); ++iter) {
		(*iter).second = (*iter).second / total;
	}

	deriveTagLambdas();
}

/**
 *  Assumes deriveTagTables has been called already. Given the counts contained
 *  in the tables filled in by deriveTagTables, it fills in the
 *  tag lambda tables.
 *
 *  Can be called as many times as needed (for instance, when we change the values
 *  of the multipliers in estimating the smoothing parameters) -- it resets the 
 *  tables at the beginning of each call. 
 */
void IdFModel::deriveTagLambdas() {
	
	if (DECODE) {
		SessionLogger::info("SERIF") << "can't derive tag lambdas during decode\n";
		return;
	}

	_tagFullLambdas->reset();
	NgramScoreTable::Table::iterator iter;
	for (iter = _tagFullHistories->get_start(); iter != _tagFullHistories->get_end(); ++iter) {
		float histories = (*iter).second;
		float uniques = _tagUniqueCounts->lookup((*iter).first);
		_tagFullLambdas->add((*iter).first, 
			histories / (histories + _tagWBMultiplier * uniques));
	}

}

/**
 * Assumes deriveTagTables() has been called and does some pre-computation of 
 * backed-off probabilities to speed-up decode. Modifies the tables, so this can
 * only be done once (enforced by TAG_PROBS_HAVE_BEEN_PRE_COMPUTED).
 */
void IdFModel::preComputeTagProbs() {

	if (DECODE) {
		SessionLogger::info("SERIF") << "can't calculate back-offs during decode\n";
		return;
	} else if (TAG_PROBS_HAVE_BEEN_PRE_COMPUTED) {
		SessionLogger::info("SERIF") << "tag probs have already been pre-computed; can't do it twice\n";
		return;
	}

	NgramScoreTable::Table::iterator iter;
	
	// tempTable will first hold unique counts...
	NgramScoreTable *tempTable = _new NgramScoreTable(1, 1000);
	for (iter = _tagReducedProbabilities->get_start(); iter != _tagReducedProbabilities->get_end(); ++iter) {
		tempTable->add((*iter).first + 1, 1);
	}

	// ...now lambdas
	for (iter = tempTable->get_start(); iter != tempTable->get_end(); ++iter) {
		float histories = _tagReducedHistories->lookup((*iter).first);
		float uniques = (*iter).second;
		(*iter).second = histories / (histories + uniques);
	}

	for (iter = _tagReducedProbabilities->get_start(); iter != _tagReducedProbabilities->get_end(); ++iter) {
		float lambda = tempTable->lookup((*iter).first + 1);
		float prob = (*iter).second;
		float prior = _tagPriors->lookup((*iter).first);
		float newprob = prob * lambda + (1 - lambda) * prior;
		(*iter).second = newprob;
	}

	Symbol ngram[2];
	for (int tag = 0; tag < _nameClassTags->getNumTags(); tag++) {
		if (_nameClassTags->getSentenceEndIndex() == tag)
			continue;
		ngram[1] = _nameClassTags->getTagSymbol(tag);
		for (int next_tag = 0; next_tag < _nameClassTags->getNumTags(); next_tag++) {
			if (_nameClassTags->getSentenceStartIndex() == next_tag)
				continue;
			if (_nameClassTags->isContinue(next_tag) &&	
				(_nameClassTags->getReducedTagSymbol(next_tag) != _nameClassTags->getReducedTagSymbol(tag)))
				continue;
			ngram[0] = _nameClassTags->getTagSymbol(next_tag);
			if (_tagReducedProbabilities->lookup(ngram) == 0) {
				float lambda = tempTable->lookup(ngram + 1);
				float prior = _tagPriors->lookup(ngram);
				float newprob = (1 - lambda) * prior;
				_tagReducedProbabilities->add(ngram, newprob);
			}
		}	
	}
	
	for (iter = _tagFullProbabilities->get_start(); iter != _tagFullProbabilities->get_end(); ++iter) {
		float lambda = _tagFullLambdas->lookup((*iter).first + 1);
		float prob = (*iter).second * lambda +
					 _tagReducedProbabilities->lookup((*iter).first) * (1 - lambda);
		(*iter).second = prob;
	}

	TAG_PROBS_HAVE_BEEN_PRE_COMPUTED = true;
}


/** 
 * @return P(tag | tag-1, word-1)
 */
double IdFModel::getTagProbability(int tag, int tag_minus_one, Symbol word_minus_one) {

	Symbol ngram[3];
	ngram[0] = _nameClassTags->getTagSymbol(tag);
	ngram[1] = _nameClassTags->getTagSymbol(tag_minus_one);
	ngram[2] = word_minus_one;
	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << ngram[0].to_debug_string() << " "
			<< ngram[1].to_debug_string() << " "
			<< ngram[2].to_debug_string() << " ";
	}
	
	float probability = _tagFullProbabilities->lookup(ngram);
	if (probability == 0)
		probability = (1 - _tagFullLambdas->lookup(ngram + 1)) 
			* _tagReducedProbabilities->lookup(ngram);
	
	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << " -- " << log(probability) << "\n";
	}
	if (probability != 0)
		return log (probability);
	else return LOG_OF_ZERO;

}

/////////////////
/// UTILITIES ///
/////////////////

/** check to make sure all tags read in are actually in _nameClassTags tag set 
 *  quickly and inefficiently implemented, so not currently called */
bool IdFModel::checkNameClassTagConsistency()
{
	if (!DECODE)
		return true;

	NgramScoreTable::Table::iterator iter;
	for (iter = _tagFullProbabilities->get_start(); iter != _tagFullProbabilities->get_end(); ++iter) {
		if (_nameClassTags->getIndexForTag((*iter).first[0]) < 0)
			return false;
	}

	for (iter = _wordTagProbabilities->get_start(); iter != _wordTagProbabilities->get_end(); ++iter) {
		if (_nameClassTags->getIndexForTag((*iter).first[1]) < 0)
			return false;
	}

	for (iter = _wordStartProbabilities->get_start(); iter != _wordStartProbabilities->get_end(); ++iter) {
		if (_nameClassTags->getIndexForTag((*iter).first[1]) < 0)
			return false;
	}

	for (iter = _wordContinueProbabilities->get_start(); iter != _wordContinueProbabilities->get_end(); ++iter) {
		if (_nameClassTags->getIndexForTag((*iter).first[1]) < 0)
			return false;
	}
		
	return true;
	
}

////////////////////////////////
////////////////////////////////
////////////////////////////////
///////  SMOOTHING  ////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////

/**
 * given a smoothing file, find the set of multipliers that maximize its likelihood.
 *
 * when completed, tables in IdFModel are re-derived with the chosen set of 
 * multipliers, so the desired model can be printed out
 */
void IdFModel::estimateLambdas(const char *smoothing_file) {
	if (DECODE) {
		SessionLogger::info("SERIF") << "can't estimate lambdas during decode\n";
		return;
	} else if (TAG_PROBS_HAVE_BEEN_PRE_COMPUTED || WORD_PROBS_HAVE_BEEN_PRE_COMPUTED) {
		SessionLogger::info("SERIF") << "can't estimate lambdas after pre-computation\n";
		return;
	}
	
	SessionLogger::info("SERIF") << "starting lambda estimation... " << std::endl;
	SessionLogger::info("SERIF") << "reading sentences... " << std::endl;


	boost::scoped_ptr<UTF8InputStream> smoothingStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& smoothingStream(*smoothingStream_scoped_ptr);
	smoothingStream.open(smoothing_file);

	// count sentences in smoothing file
	int sentence_count = 0;
	IdFSentence *smoothingSentence = _new IdFSentence(_nameClassTags);
	while (smoothingSentence->readTrainingSentence(smoothingStream)) {
		sentence_count++;
	}
	smoothingStream.close();

	// fill in sentence set from smoothing file
	boost::scoped_ptr<UTF8InputStream> smoothingStream2_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& smoothingStream2(*smoothingStream2_scoped_ptr);
	IdFSentence **sentences = _new IdFSentence *[sentence_count];
	smoothingStream2.open(smoothing_file);
	for (int i = 0; i < sentence_count; i++) {
		sentences[i] = _new IdFSentence(_nameClassTags);
		sentences[i]->readTrainingSentence(smoothingStream2);
		for (int j = 0; j < sentences[i]->getLength(); j++) {
			if (!wordIsInVocab(sentences[i]->getWord(j))) {
				sentences[i]->setWord(j, 
					getWordFeatures(sentences[i]->getWord(j), j == 0,
					wordIsInVocab(getNormalizedWord(sentences[i]->getWord(j)))));
			}
		}
	}
	smoothingStream2.close();
	SessionLogger::info("SERIF") << "TAGS..." << std::endl;

	// try every _tagWBMultiplier in a certain range; find the one that
	// maximizes the likelihood of the tags in the smoothing corpus
	float best_tagWBMultiplier = 1;
	_best_tag_smoothing_score = -100000;
	for (_tagWBMultiplier = 0; _tagWBMultiplier < 6; _tagWBMultiplier += .25) {
		deriveTagLambdas();
		SessionLogger::info("SERIF") << _tagWBMultiplier << ": ";
		double this_tag_score = sumTagProbabilities(sentences, sentence_count);
		SessionLogger::info("SERIF") << this_tag_score << std::endl;
		if (this_tag_score > _best_tag_smoothing_score) {
			_best_tag_smoothing_score = this_tag_score;
			best_tagWBMultiplier = _tagWBMultiplier;
		}
	}

	SessionLogger::info("SERIF") << "best tag k = " << best_tagWBMultiplier << std::endl;
	_tagWBMultiplier = best_tagWBMultiplier;
	// do a final lambda derivation with the multiplier set to chosen value
	deriveTagLambdas();

	SessionLogger::info("SERIF") << "WORDS..." << std::endl;

	// try every set of multipliers in a certain range; find the set that
	// maximizes the likelihood of the words in the smoothing corpus
	float best_wordStartWBMultiplier = 1;
	float best_wordContinueWBMultiplier = 1;
	float best_wordTagWBMultiplier = 1;
	float best_wordReducedTagWBMultiplier = 1;
	_best_word_smoothing_score = -1000000;
	for (_wordStartWBMultiplier = 1; _wordStartWBMultiplier < 10; _wordStartWBMultiplier += 1) {
		SessionLogger::info("SERIF") << _wordStartWBMultiplier << std::endl;
		for (_wordContinueWBMultiplier = 1; _wordContinueWBMultiplier < 10; _wordContinueWBMultiplier += 1) {
			SessionLogger::info("SERIF") << "  " << _wordContinueWBMultiplier << std::endl;
			for (_wordTagWBMultiplier = 1; _wordTagWBMultiplier < 10; _wordTagWBMultiplier += 1) {
				SessionLogger::info("SERIF") << "    " << _wordTagWBMultiplier << std::endl;
				for (_wordReducedTagWBMultiplier = 1; _wordReducedTagWBMultiplier < 10; _wordReducedTagWBMultiplier += 1) {
					deriveWordLambdas();
					double this_word_score = sumWordProbabilities(sentences, sentence_count);
					if (this_word_score > _best_word_smoothing_score) {
						_best_word_smoothing_score = this_word_score;
						best_wordStartWBMultiplier = _wordStartWBMultiplier;
						best_wordContinueWBMultiplier = _wordContinueWBMultiplier;
						best_wordTagWBMultiplier = _wordTagWBMultiplier;
						best_wordReducedTagWBMultiplier = _wordReducedTagWBMultiplier;
					}
				}
			}
		}
	}

	_wordStartWBMultiplier = best_wordStartWBMultiplier;
	_wordContinueWBMultiplier = best_wordContinueWBMultiplier;
	_wordTagWBMultiplier = best_wordTagWBMultiplier;
	_wordReducedTagWBMultiplier = best_wordReducedTagWBMultiplier;
	SessionLogger::info("SERIF") << "best start k: " << _wordStartWBMultiplier << std::endl;
	SessionLogger::info("SERIF") << "best continue k: " << _wordContinueWBMultiplier << std::endl;
	SessionLogger::info("SERIF") << "best wordtag k: " << _wordTagWBMultiplier << std::endl;
	SessionLogger::info("SERIF") << "best wordreducedtag k: " << _wordReducedTagWBMultiplier << std::endl;	
	// do a final lambda derivation with the multipliers set to their chosen values
	deriveWordLambdas();

}

/** 
 * Calculates the likelihood of generating all the tags contained in the given
 * sentence set, according to the current probability tables. So: walks through 
 * all the sentences, accumulating the log probabilities associated with 
 * generating every tag it sees. If the sum ever gets to be lower than the current 
 * best summed score (_best_tag_smoothing_score), return immediately.
 *
 * @param sentences set of sentences over which to sum tag log probabilities
 * @param sentence_count number of sentences in set
 */
double IdFModel::sumTagProbabilities(IdFSentence **sentences, int sentence_count) {

	double score = 0;
	for (int i = 0; i < sentence_count; i++) {
		score += getTagProbabilityFromScratch(sentences[i]->getTag(0),
				_nameClassTags->getSentenceStartIndex(), _nameClassTags->getSentenceStartTag());
		int length = sentences[i]->getLength();
		for (int j = 1; j < length; j++) {
			score += getTagProbabilityFromScratch(sentences[i]->getTag(j), 
				sentences[i]->getTag(j-1), sentences[i]->getWord(j-1));
		}
		score += getTagProbabilityFromScratch(_nameClassTags->getSentenceEndIndex(),
				sentences[i]->getTag(length-1), sentences[i]->getWord(length-1));
		if (_best_tag_smoothing_score > score)
			return score;
	}

	return score;

}

/** 
 * Calculates the likelihood of generating all the words contained in the given
 * sentence set, according to the current probability tables. So: walks through 
 * all the sentences, accumulating the log probabilities associated with 
 * generating every word it sees. If the sum ever gets to be lower than the current 
 * best summed score (_best_word_smoothing_score), return immediately.
 *
 * @param sentences set of sentences over which to sum word log probabilities
 * @param sentence_count number of sentences in set
 */
double IdFModel::sumWordProbabilities(IdFSentence **sentences, int sentence_count) {
	double score = 0;
	for (int i = 0; i < sentence_count; i++) {
		score += getWordProbabilityFromScratch(sentences[i]->getWord(0),
			sentences[i]->getTag(0), _nameClassTags->getSentenceStartIndex(), 
			_nameClassTags->getSentenceStartTag());
		int length = sentences[i]->getLength();
		for (int j = 1; j < length; j++) {
			score += getWordProbabilityFromScratch(sentences[i]->getWord(j),
				sentences[i]->getTag(j),
				sentences[i]->getTag(j-1), 
				sentences[i]->getWord(j-1));
		}
		if (_best_word_smoothing_score > score)
			return score;
	}

	SessionLogger::info("SERIF") << "      " << _wordReducedTagWBMultiplier << ": ";
	SessionLogger::info("SERIF") << score << std::endl;
	return score;


}

/** 
 * Assumes that preComputeWordProbs() has not been called, but this is NOT
 * enforced (didn't want to to have to check for each call). Currently only
 * called in estimateLambdas(), which does check. But be careful.
 * 
 * @return P(word | tag, tag-1, word-1)
 */
double IdFModel::getWordProbabilityFromScratch(Symbol word, int tag, 
									int tag_minus_one, Symbol word_minus_one) 
{
    
	Symbol ngram[3];
	ngram[0] = word;
	ngram[1] = _nameClassTags->getTagSymbol(tag);

	float full_probability;
	float lambda1;
	if (_nameClassTags->isContinue(tag)) {
		ngram[2] = word_minus_one;
		full_probability = _wordContinueProbabilities->lookup(ngram);
		lambda1 = _wordContinueLambdas->lookup(ngram + 1);
		if (OUTPUT_ALL_PROBS) {
			SessionLogger::info("SERIF") << ngram[0].to_debug_string() << " "
				<< ngram[1].to_debug_string() << " "
				<< ngram[2].to_debug_string() << " ";
		}
	} else {
		ngram[2] = _nameClassTags->getTagSymbol(tag_minus_one);
		full_probability = _wordStartProbabilities->lookup(ngram);
		lambda1 = _wordStartLambdas->lookup(ngram + 1);
		if (OUTPUT_ALL_PROBS) {
			SessionLogger::info("SERIF") << ngram[0].to_debug_string() << " "
				<< ngram[1].to_debug_string() << " "
				<< ngram[2].to_debug_string() << " ";
		}
	}

	float word_tag_probability = _wordTagProbabilities->lookup(ngram);
	float lambda2 = _wordTagLambdas->lookup(ngram + 1);

	ngram[1] = _nameClassTags->getReducedTagSymbol(tag);
	float reduced_probability = _wordReducedTagProbabilities->lookup(ngram);
	float lambda3 = _wordReducedTagLambdas->lookup(ngram + 1);

	float probability 
		= lambda1      * full_probability + 
		((1 - lambda1) * (lambda2      * word_tag_probability +
						((1 - lambda2) * (lambda3 * reduced_probability +
										 (1 - lambda3) * _oneOverVocabSize))));

	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << " -- " << log(probability) << "\n";
	}
	if (probability != 0)
		return log (probability);
	else return LOG_OF_ZERO;
}

/** 
 * Assumes that preComputeTagProbs() has not been called, but this is NOT
 * enforced (didn't want to to have to check for each call). Currently only
 * called in estimateLambdas(), which does check. But be careful.
 * 
 * @return P(tag | tag-1, word-1)
 */
double IdFModel::getTagProbabilityFromScratch(int tag, int tag_minus_one, Symbol word_minus_one) {

	Symbol ngram[3];
	ngram[0] = _nameClassTags->getTagSymbol(tag);
	ngram[1] = _nameClassTags->getTagSymbol(tag_minus_one);
	ngram[2] = word_minus_one;
	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << ngram[0].to_debug_string() << " "
			<< ngram[1].to_debug_string() << " "
			<< ngram[2].to_debug_string() << " ";
	}

	float full_probability = _tagFullProbabilities->lookup(ngram);
	float lambda = _tagFullLambdas->lookup(ngram + 1);
	float reduced_probability = _tagReducedProbabilities->lookup(ngram);

	float probability = lambda * full_probability + (1 - lambda) * reduced_probability;

	if (OUTPUT_ALL_PROBS) {
		SessionLogger::info("SERIF") << " -- " << log(probability) << "\n";
	}
	if (probability != 0)
		return log (probability);
	else return LOG_OF_ZERO;

}

void IdFModel::verifyEntityTypes(char *file_name) {
	boost::scoped_ptr<UTF8InputStream> tagProbsStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& tagProbsStream(*tagProbsStream_scoped_ptr);
	tagProbsStream.open(file_name);

	if (tagProbsStream.fail()) {
		throw UnexpectedInputException(
			"IdFModel::verifyEntityTypes()",
			"Unable to open IdentiFinder (name finder) model files.");
	}

	// skip the sections we don't care about
	int n_entries;
	std::wstring line;
	tagProbsStream >> n_entries;
	for (int i = 0; i < n_entries + 1; i++)
		tagProbsStream.getLine(line);
	tagProbsStream >> n_entries;
	for (int j = 0; j < n_entries + 1; j++)
		tagProbsStream.getLine(line);

	tagProbsStream >> n_entries;
	for (int k = 0; k < n_entries; k++) {
		UTF8Token token;
		tagProbsStream >> token; // (
		tagProbsStream >> token; // (

		tagProbsStream >> token; // enitity type plus ST/CO
		wchar_t etype_str[100];
		wcsncpy(etype_str, token.chars(), 99);
		wchar_t *hyphen = wcschr(etype_str, L'-');
		if (hyphen != 0) {
			*hyphen = L'\0';
			Symbol etype(etype_str);
			try {
				if (wcscmp(etype_str, L"NONE"))
					EntityType(Symbol(etype_str));
			}
			catch (UnexpectedInputException &) {
				std::string message = "";
				message += "The IdentiFinder (name finder) model refers to "
						"at least one unrecognized entity type: ";
				message += etype.to_debug_string();
				throw UnexpectedInputException(
					"IdFModel::verifyEntityTypes()",
					message.c_str());
			}
		}

		tagProbsStream >> token; // another tag
		tagProbsStream >> token; // )
		tagProbsStream >> token; // prob
		tagProbsStream >> token; // )
	}

	tagProbsStream.close();
}


