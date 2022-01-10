
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include "Generic/common/Symbol.h"

#include "DistributionalUtil.h"


float DistributionalUtil::calculateAvgScore(const float& s1, const float& s2) {
	float avgS = -1;

	if( s1!=-1 && s2!=-1 ) 
		avgS = (s1 + s2)/2;
		
	return avgS;
}

int DistributionalUtil::scoreToBin(const float& score) {
	int bin;

        if(score<0.5) {
        	bin = 5;
        }
        else if( (0.5<=score) && (score<0.6) ) {
                bin = 6;
        }
        else if( (0.6<=score) && (score<0.7) ) {
                bin = 7;
        }
        else if( (0.7<=score) && (score<0.8) ) {
                bin = 8;
        }
        else if( (0.8<=score) && (score<0.9) ) {
                bin = 9;
                }
        else if(0.9<=score) {
                bin = 10;
        }

	return bin;
}

/*
  10/15/2013 : Yee Seng Chan
  The scoreToBins is temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as a single bin as currently represented by the scoreToBin function.

std::vector<std::wstring> DistributionalUtil::scoreToBins(const float& score) {
	std::vector<std::wstring> bins;

        if(score<0.5) {
        	bins.push_back(L"<=0");
        }
        else if( (0.5<=score) && (score<0.6) ) {
                bins.push_back(L">0");
                bins.push_back(L"<=5");
        }
        else if( (0.6<=score) && (score<0.7) ) {
                bins.push_back(L">0");
                bins.push_back(L">5");
                bins.push_back(L"<=6");
        }
        else if( (0.7<=score) && (score<0.8) ) {
                bins.push_back(L">0");
                bins.push_back(L">5");
                bins.push_back(L">6");
                bins.push_back(L"<=7");
        }
        else if( (0.8<=score) && (score<0.9) ) {
                bins.push_back(L">0");
                bins.push_back(L">5");
                bins.push_back(L">6");
                bins.push_back(L">7");
                bins.push_back(L"<=8");
        }
        else if(0.9<=score) {
                bins.push_back(L">0");
                bins.push_back(L">5");
                bins.push_back(L">6");
                bins.push_back(L">7");
                bins.push_back(L">8");
        }

	return bins;
}
*/

Symbol DistributionalUtil::determinePosType(const Symbol& posTag) {
	if(boost::algorithm::starts_with(posTag.to_string(), L"NN"))
		return Symbol(L"N");
	else if(boost::algorithm::starts_with(posTag.to_string(), L"JJ"))
		return Symbol(L"J");
	else if(boost::algorithm::starts_with(posTag.to_string(), L"VB"))
		return Symbol(L"V");
	else
		return Symbol();

	return Symbol();
}

