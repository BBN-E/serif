// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MORPH_MODEL_H
#define MORPH_MODEL_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"

class MorphologicalAnalyzer;

class MorphModel {
public:
	/** Create and return a new MorphModel. */
	static MorphModel *build(const char* model_file_prefix) { return _factory()->build(model_file_prefix); }
	static MorphModel *build() { return _factory()->build(); }
	/** Hook for registering new MorphModel factories */
	struct Factory { 
		virtual MorphModel *build(const char* model_file_prefix) = 0; 
		virtual MorphModel *build() = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


	virtual ~MorphModel();	

	virtual void trainOnSingleFile(const char *train_file, const char *model_prefix);
	virtual void setMultiplier(int m) { _mult = m; };

	virtual Symbol getTrainingWordFeatures(Symbol word) = 0;
	virtual Symbol getTrainingReducedWordFeatures(Symbol word) = 0;

	virtual int readSentence(UTF8InputStream& stream, Symbol* sentence, int num_words);

	bool wordIsInVocab(Symbol word){ return _vocabTable->lookup(&word) == 1; };
	double getWordProbability(Symbol word, Symbol prev_word);
	double getWordProbability(Symbol word, Symbol prev_word, double scores[]);

	static const double _LOG_OF_ZERO;
	static const Symbol _START;
	static const Symbol _END;

protected:
	MorphModel();
	MorphModel(const char* model_file_prefix);

private:
	NgramScoreTable* _unigramProbTable;
	NgramScoreTable* _bigramProbTable;
	NgramScoreTable* _uniqueHistoryCount;
	NgramScoreTable* _lambdaTable;

	int _mult;
	int _unknown_threshold;
	
	//training tables
	NgramScoreTable* _vocabTable;
	NgramScoreTable* _knownVocabTable;
	NgramScoreTable* _featureTable;	//count the number of unique words that have feature x
	NgramScoreTable* _unigramCount;
	NgramScoreTable* _bigramCount;
	NgramScoreTable* _featureWordPairs;
	NgramScoreTable* _uniqFeatureCounts;
	NgramScoreTable* _featureCounts;

	NgramScoreTable* _bigramFeatureWordPairs;
	NgramScoreTable* _bigramUniqFeatureCounts;
	NgramScoreTable* _bigramFeatureCounts;

	float _numberOfTrainingWords;

	int getVocab(UTF8InputStream& uis);
	void collectCounts(UTF8InputStream& uis);
	void deriveProb();
	void deriveLambda();

	void printTables(const char *model_file_prefix);	

protected:
	bool containsDigit(Symbol wordSym) const;
	bool containsASCII(Symbol wordSym) const;
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/morphSelection/ar_MorphModel.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/morphology/kr_MorphModel.h"
//#else
//	#include "Generic/morphSelection/xx_MorphModel.h"
//#endif

#endif
