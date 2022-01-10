// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/theories/Token.h"
#include "Generic/common/NgramScoreTable.h"


const Symbol TokenObservation::_className(L"token");

DTObservation *TokenObservation::makeCopy() {
	TokenObservation *copy = _new TokenObservation();

	copy->populate(_token, _lcSymbol, _idfWordFeature, _wordClass, _allIDFWordFeatures, _n_idf_features);
	copy->_n_decoder_features = _n_decoder_features;
	for(int i =0; i< _n_decoder_features; i++){
		copy->_decoderFeatures[i] = _decoderFeatures[i];
	}	
	//need to update with decoder features
	return copy;
}

//void TokenObservation::populate(const Token &token, Symbol lcSymbol,
//				Symbol idfWordFeature, const WordClusterClass &wordClass,
//				Symbol* featureArray, int n_idf_features)
//{
//		populate(token, lcSymbol,idfWordFeature, wordClass,
//				featureArray, n_idf_features, TEMP_WORD_COUNT);
//}

void TokenObservation::populate(const Token &token, Symbol lcSymbol,
				Symbol idfWordFeature, const WordClusterClass &wordClass,
				Symbol* featureArray, int n_idf_features)
{
	populate(token, lcSymbol, idfWordFeature, wordClass,featureArray,n_idf_features,0,0);
}
void TokenObservation::populate(const Token &token, Symbol lcSymbol,
				Symbol idfWordFeature, const WordClusterClass &wordClass,
				Symbol* featureArray, int n_idf_features, int wordCount,
				NgramScoreTable* bigram_scores)
{
	_token = token;
	_symbol = _token.getSymbol();
	_lcSymbol = lcSymbol;
	_idfWordFeature = idfWordFeature;
	_wordClass = wordClass;
	_n_decoder_features = 0;
	if (n_idf_features > DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
		n_idf_features = DTFeatureType::MAX_FEATURES_PER_EXTRACTION;
	_n_idf_features = n_idf_features;
	for(int i =0; i< _n_idf_features; i++){
		_allIDFWordFeatures[i] = featureArray[i];
	}
	_bigram_scores = bigram_scores;
	_wordCount = wordCount;
}

void TokenObservation::dump(){
	std::cout<<"Word: "<<_token.getSymbol().to_debug_string();
}

std::string TokenObservation::toString() const {
	return std::string(_token.getSymbol().to_debug_string());
}

void TokenObservation::addDecoderFeatures(Symbol decoderName, Symbol tag){
	wchar_t buffer[500];
	wcscpy(buffer, decoderName.to_string());
	wcscat(buffer, L":");
	wcscat(buffer, tag.to_string());
	if(_n_decoder_features < MAX_DECODERS){
		_decoderFeatures[_n_decoder_features++] = Symbol(buffer);
	}
}

void TokenObservation::updateCacheFeatureType2match(Symbol featureName, bool isMatch) {
	_cacheFeatureType2match[featureName] = isMatch;
}

bool TokenObservation::isMatch(Symbol featureName) {
	return _cacheFeatureType2match[featureName];
}
