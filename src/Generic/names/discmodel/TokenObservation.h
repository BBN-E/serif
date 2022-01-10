// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_OBSERVATION_H
#define TOKEN_OBSERVATION_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/hash_map.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/Token.h"
#include "Generic/common/NgramScoreTable.h"

#define MAX_DECODERS 10

/** A TokenObservation represents all the information about a token that the
  * Discriminative-IdF extracts features from.
  */

class TokenObservation : public DTObservation {
public:
	TokenObservation()
		: DTObservation(_className),
		  _token(Symbol()), _symbol(Symbol()), _lcSymbol(Symbol()),
		  _idfWordFeature(Symbol()),
		  _wordClass(WordClusterClass::nullCluster()), _n_decoder_features(0),
		  _bigram_scores(0)
	{}

	virtual DTObservation *makeCopy();

	/// Recycle the instance by entering new information into it.
//	void populate(const Token &token, Symbol lcSymbol,
//				  Symbol idfWordFeature, const WordClusterClass &wordClass,
//		  Symbol* featureArray, int n_idf_features);

    // added new information source of counts
	// xxx: need to make the active learning work with it as well
	void populate(const Token &token, Symbol lcSymbol,
				  Symbol idfWordFeature, const WordClusterClass &wordClass,
				  Symbol* featureArray, int n_idf_features,
				  int wordCount, NgramScoreTable* bigram_scores=0);
	// temp for active learning
	void populate(const Token &token, Symbol lcSymbol,
				  Symbol idfWordFeature, const WordClusterClass &wordClass,
				  Symbol* featureArray, int n_idf_features);

	// for list matching
	void updateCacheFeatureType2match(Symbol featureName, bool isMatch);
	bool isMatch(Symbol featureName);
	//

	/// May return null pointer
	const Token &getToken() const { return _token; }
	Symbol getSymbol() const { return _symbol; }
	Symbol getLCSymbol() const { return _lcSymbol; }
	Symbol getIdFWordFeature() const { return _idfWordFeature; }
	int getNDecoderFeatures(){return _n_decoder_features;}
	const Symbol* getDecoderFeatures(){return _decoderFeatures;}
	int getNIDFWordFeatures(){return _n_idf_features;}
	Symbol getNthIDFWordFeature(int n){return _allIDFWordFeatures[n];}
	const WordClusterClass &getWordClass() const { return _wordClass; }
	void addDecoderFeatures(Symbol decoderName, Symbol tag);
	void dump();
	std::string toString() const;
	int getWordCount(){ return _wordCount; }
	NgramScoreTable* getBigramCountTable() { return _bigram_scores; }

private:
	Token _token;
	Symbol _symbol;
	Symbol _lcSymbol;
	Symbol _idfWordFeature;
	Symbol _allIDFWordFeatures[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	int _n_idf_features;
	WordClusterClass _wordClass;
	Symbol _decoderFeatures[MAX_DECODERS];
    int _n_decoder_features;
	int _wordCount;
	NgramScoreTable* _bigram_scores;

	// for list matching
	Symbol::HashMap<bool> _cacheFeatureType2match;
	//

	static const Symbol _className;
	static const  int TEMP_WORD_COUNT= 0; // to check the counts - ***** remove later
};

#endif
