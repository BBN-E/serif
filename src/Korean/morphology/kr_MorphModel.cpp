// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "common/Symbol.h"
#include "morphSelection/KoreanMorphModel.h"


KoreanMorphModel::KoreanMorphModel(const char* model_prefix, KoreanMorphologicalAnalyzer *morphAnalyzer)
: MorphModel(model_prefix) {}

Symbol KoreanMorphModel::getTrainingWordFeatures(Symbol word){
	
	if(containsDigit(word)) return Symbol(L":NUMBER");
	if(containsASCII(word)) return Symbol(L":ASCII");

	return Symbol(L":OTHER");
}

Symbol KoreanMorphModel::getTrainingReducedWordFeatures(Symbol word){
	
	if(containsDigit(word)) return Symbol(L":NUMBER");
	if(containsASCII(word)) return Symbol(L":ASCII");
	
	return Symbol(L":OTHER");
}
