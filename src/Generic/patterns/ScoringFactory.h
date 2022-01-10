// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef SCORING_FACTORY_H
#define SCORING_FACTORY_H

#include <vector>
#include "common/Symbol.h"

class ScoringFactory {
public:

	static float scoreMax(std::vector<float> scoreVector, float top_score);	
	static float scoreMin(std::vector<float> scoreVector, float top_score);
	static float scoreMult(std::vector<float> scoreVector, float top_score);
	static float scoreMultIncludingTop(std::vector<float> scoreVector, float top_score);
	static float scoreAvg(std::vector<float> scoreVector, float top_score);

	static Symbol maxSym;
	static Symbol minSym;
	static Symbol multSym;
	static Symbol multIncludingTopSym;
	static Symbol avgSym;
	
	typedef float(*ScoreFunctionPtr)(std::vector<float>, float);
	static ScoreFunctionPtr getScoringFunction(Symbol sym);	
	static ScoreFunctionPtr getDefaultScoringFunction() { return scoreMax; }

};

#endif

