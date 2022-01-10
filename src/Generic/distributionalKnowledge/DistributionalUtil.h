
#ifndef DISTRIBUTIONAL_UTIL_H
#define DISTRIBUTIONAL_UTIL_H

#include <string>
#include <vector>

class Symbol;

/*
  10/15/2013 : Yee Seng Chan
  The scoreToBins is temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as a single bin as currently represented by the scoreToBin function.
*/

namespace DistributionalUtil {

	// given two scores, calculate their average. But if either one is -1, then return -1
        float calculateAvgScore(const float& s1, const float& s2);

	// remap score to bin
        int scoreToBin(const float& score);
        //std::vector<std::wstring> scoreToBins(const float& score);

	// returns 'N' 'V' 'J' if posTag is a noun, verb, or adjective. Else, return null symbol
        Symbol determinePosType(const Symbol& posTag);
};

#endif

