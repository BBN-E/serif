// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/common/UnexpectedInputException.h"
#include <sstream>

Symbol ScoringFactory::maxSym = Symbol(L"max");
Symbol ScoringFactory::minSym = Symbol(L"min");
Symbol ScoringFactory::multSym = Symbol(L"mult");
Symbol ScoringFactory::multIncludingTopSym = Symbol(L"mult-including-top");
Symbol ScoringFactory::avgSym = Symbol(L"avg");


float ScoringFactory::scoreMax(std::vector<float> scoreVector, float top_score) {
	if (top_score != Pattern::UNSPECIFIED_SCORE)
		return top_score;
	std::vector<float>::iterator iter;
	float value = 0;
	bool some_value = false;
	for (iter = scoreVector.begin(); iter != scoreVector.end(); iter++) {
		if (*iter == Pattern::UNSPECIFIED_SCORE)
			continue;
		value = std::max(value, *iter);
		some_value = true;
	}
	if (some_value)
		return value;
	else return Pattern::UNSPECIFIED_SCORE;
}

float ScoringFactory::scoreMin(std::vector<float> scoreVector, float top_score) {
	if (top_score != Pattern::UNSPECIFIED_SCORE)
		return top_score;
	std::vector<float>::iterator iter;
	bool some_value = false;
	float value = 1;
	for (iter = scoreVector.begin(); iter != scoreVector.end(); iter++) {
		if (*iter == Pattern::UNSPECIFIED_SCORE)
			continue;
		value = std::min(value, *iter);
		some_value = true;
	}
	if (some_value)
		return value;
	else return Pattern::UNSPECIFIED_SCORE;
}	

float ScoringFactory::scoreMult(std::vector<float> scoreVector, float top_score) {
	if (top_score != Pattern::UNSPECIFIED_SCORE)
		return top_score;
	std::vector<float>::iterator iter;
	bool some_value = false;
	float value = 1;
	for (iter = scoreVector.begin(); iter != scoreVector.end(); iter++) {
		if (*iter == Pattern::UNSPECIFIED_SCORE)
			continue;
		value *= *iter;
		some_value = true;
	}
	if (some_value)
		return value;
	else return Pattern::UNSPECIFIED_SCORE;
}

float ScoringFactory::scoreMultIncludingTop(std::vector<float> scoreVector, float top_score) {
	std::vector<float>::iterator iter;
	bool some_value = false;
	float value = 1;
	for (iter = scoreVector.begin(); iter != scoreVector.end(); iter++) {
		if (*iter == Pattern::UNSPECIFIED_SCORE)
			continue;
		value *= *iter;
		some_value = true;
	}
	// this is what makes this function special
	if (top_score != Pattern::UNSPECIFIED_SCORE) {
		value *= top_score;
		some_value = true;
	}

	if (some_value)
		return value;
	else return Pattern::UNSPECIFIED_SCORE;
}

float ScoringFactory::scoreAvg(std::vector<float> scoreVector, float top_score) {
	if (top_score != Pattern::UNSPECIFIED_SCORE)
		return top_score;
	std::vector<float>::iterator iter;
	float value = 0;
	int n_values = 0;
	for (iter = scoreVector.begin(); iter != scoreVector.end(); iter++) {
		if (*iter == Pattern::UNSPECIFIED_SCORE)
			continue;
		value += *iter;
		n_values++;
	}
	if (n_values != 0)
		return value / n_values;
	else return Pattern::UNSPECIFIED_SCORE;
}

ScoringFactory::ScoreFunctionPtr ScoringFactory::getScoringFunction(Symbol sym) {
	if (sym == maxSym)
		return scoreMax;
	else if (sym == minSym)
		return scoreMin;
	else if (sym == multSym)
		return scoreMult;
	else if (sym == multIncludingTopSym)
		return scoreMultIncludingTop;
	else if (sym == avgSym)
		return scoreAvg;
	else {
		std::stringstream error;
		error << "Invalid scoring function (must be max, min, avg, mult, or mult-including-top): " << sym.to_debug_string();
		throw UnexpectedInputException("ScoringFactory::getScoringFunction()", error.str().c_str());		
	}
}
