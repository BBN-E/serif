// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef IDF_MODEL_H
#define IDF_MODEL_H

#include "Generic/common/NgramScoreTable.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/names/IdFSentence.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"

/**
 * Core IdF model. 
 * 
 * During decode, contains probability tables, lambda tables, and vocab table.
 * During training, contains those, plus other intermediate tables and constants.
 * Some consistency is enforced by setting DECODE = true during decode and having
 * functions dependent on training information return immediately and complain when 
 * called during decode. 
 *
 * Also note that before we print out the filled-in tables, we do some pre-computation
 * of the probabilities w/ their backoffs. This can only be done once, as it modifies
 * the tables. This is enforced by TAG_PROBS_HAVE_BEEN_PRE_COMPUTED and 
 * WORD_PROBS_HAVE_BEEN_PRE_COMPUTED, but it is important to be aware of.
 *
 * Note that word features are only available to the outside world by coming through
 * IdFModel; _wordFeatures is a private member of this class.
 */
class IdFModel {
private:
	// just to make sure everything is consistent
	bool const DECODE;
	bool TAG_PROBS_HAVE_BEEN_PRE_COMPUTED;
	bool WORD_PROBS_HAVE_BEEN_PRE_COMPUTED;

	float _tagWBMultiplier;
	float _wordStartWBMultiplier;
	float _wordContinueWBMultiplier;
	float _wordTagWBMultiplier;
	float _wordReducedTagWBMultiplier;

	float _oneOverVocabSize;
	
	NgramScoreTable* _tagFullProbabilities;
	NgramScoreTable* _tagFullLambdas;
	NgramScoreTable* _tagReducedProbabilities;
	NgramScoreTable* _tagReducedHistories;
	NgramScoreTable* _tagPriors;

	NgramScoreTable* _tagFullHistories;
	NgramScoreTable* _tagUniqueCounts;
	
	NgramScoreTable* _wordStartLambdas;
	NgramScoreTable* _wordStartProbabilities;
	NgramScoreTable* _wordContinueLambdas;
	NgramScoreTable* _wordContinueProbabilities;
	NgramScoreTable* _wordTagLambdas;
	NgramScoreTable* _wordTagProbabilities;
	NgramScoreTable* _wordReducedTagLambdas;
	NgramScoreTable* _wordReducedTagProbabilities;

	NgramScoreTable* _wordStartHistories;
	NgramScoreTable* _wordContinueHistories;
	NgramScoreTable* _wordTagHistories;
	NgramScoreTable* _wordReducedTagHistories;
	NgramScoreTable* _startUniqueCounts;
	NgramScoreTable* _continueUniqueCounts;
	NgramScoreTable* _wordTagUniqueCounts;
	NgramScoreTable* _wordReducedTagUniqueCounts;

	NgramScoreTable* _preComputedFirstLevelLambdas;
	NgramScoreTable* _preComputedSecondLevelLambdas;
	NgramScoreTable* _preComputedThirdLevelLambdas;

	NgramScoreTable* _vocabularyTable;
	const NameClassTags *_nameClassTags;
	const IdFWordFeatures *_wordFeatures;

	double sumTagProbabilities(IdFSentence **pairs, int sentence_count);
	double sumWordProbabilities(IdFSentence **pairs, int sentence_count);
	void deriveWordLambdas();
	void deriveTagLambdas();
	void preComputeTagProbs();
	void preComputeWordProbs();

	double _best_tag_smoothing_score;
	double _best_word_smoothing_score;

	// THESE USE THE TABLES WITH THE RAW PROBABILITIES IN THEM 
	//   (I.E. BEFORE THERE IS ANY "PRE-COMPUTATION")
	/** return probability of generating word, derived from scratch 
	  * -- only for use in estimating smoothing parameters */
	double getWordProbabilityFromScratch(Symbol word, int tag, 
		int tag_minus_one, Symbol word_minus_one);
	/** return probability of generating tag, derived from scratch
	  * -- only for use in estimating smoothing parameters */
	double getTagProbabilityFromScratch(int tag, 
		int tag_minus_one, Symbol word_minus_one);

	void verifyEntityTypes(char *file_name);

	static Symbol NOT_SEEN;

public:
	IdFModel(const NameClassTags *nameClassTags, 
		const IdFWordFeatures *wordFeatures = 0);
	IdFModel(const NameClassTags *nameClassTags, const char *model_file_prefix, 
		const IdFWordFeatures *wordFeatures = 0);
	bool checkNameClassTagConsistency();
	void calculateOneOverVocabSize();
	void setWordFeatures (IdFWordFeatures *features) { //added mseldin to allow external setting of language dependencies
		_wordFeatures=features;
	}
	void setKValues(float tag_k, float word_start_k, float word_continue_k, 
		float word_tag_k, float word_reduced_tag_k) 
	{
		_tagWBMultiplier = tag_k;
		_wordStartWBMultiplier = word_start_k;
		_wordContinueWBMultiplier = word_continue_k;
		_wordTagWBMultiplier = word_tag_k;
		_wordReducedTagWBMultiplier = word_reduced_tag_k;
	}

	const double LOG_OF_ZERO;

	void deriveWordTables(NgramScoreTable* dataTable);
	void deriveTagTables(NgramScoreTable* dataTable);
	void estimateLambdas(const char *smoothing_file);
	void printTables(const char *model_file_prefix);

	/** return probability of generating word */
	double getWordProbability(Symbol word, int tag, 
		int tag_minus_one, Symbol word_minus_one);
	/** return probability of generating tag */
	double getTagProbability(int tag, 
		int tag_minus_one, Symbol word_minus_one);

	/** add word to _vocabularyTable */
	void addToVocab(Symbol word) {
		_vocabularyTable->add(&word);
	}

	/** return true if word is in _vocabularyTable */
	bool wordIsInVocab(Symbol word) {
		return (_vocabularyTable->lookup(&word) != 0);
	}

	/** return true if normalized word is in _vocabularyTable */
	bool normalizedWordIsInVocab(Symbol word) {
                Symbol nword = getNormalizedWord(word);
		return (_vocabularyTable->lookup(&nword) != 0);
	}

	/** return word frequency */
	float getWordFrequency(Symbol word) {
		return _vocabularyTable->lookup(&word);
	}

	// these functions pass calls through to _wordFeatures

	Symbol getWordFeatures(Symbol word, bool first_word, bool normalized_word_is_in_vocab) {
		return _wordFeatures->features(word, first_word, normalized_word_is_in_vocab);
	}

	Symbol getNormalizedWord(Symbol word) {
		return _wordFeatures->normalizedWord(word);
	}

	bool wordContainsDigit(Symbol word) {
		return _wordFeatures->containsDigit(word);
	}

	bool useLists() {
		return (_wordFeatures->getListSet() != 0);
	}

	int isListMember(IdFSentenceTokens *tokens, int start_index) {
		return _wordFeatures->getListSet()->isListMember(tokens, start_index);
	}

	Symbol barSymbols(IdFSentenceTokens *tokens, int start_index, int length) {
		return _wordFeatures->getListSet()->barSymbols(tokens, start_index, length);
	}

	NgramScoreTable * getTagReducedProbabilities() { //can be used prior to decoding for sanity checking name class tag file to those in model file
		return _tagReducedProbabilities;
	}
};





#endif
