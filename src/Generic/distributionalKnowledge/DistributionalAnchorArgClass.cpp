
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <algorithm>

#include "Generic/common/Symbol.h"
#include "DistributionalKnowledgeTable.h"
#include "DistributionalAnchorArgClass.h"
#include "DistributionalUtil.h"

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

DistributionalAnchorArgClass::DistributionalAnchorArgClass(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag): 
								_anchor(anchor), _arg(arg) {
	_subScore = -1;
	_objScore = -1;
	_avgSubObjScore = -1;
	_maxSubObjScore = -1;

	_subScoreSB = 0;
	_objScoreSB = 0;
	_avgSubObjScoreSB = 0;
	_maxSubObjScoreSB = 0;

	_anchorArgSim = -1;
	_anchorArgSimSB = 0;

	_anchorArgPmi = -1;
	_anchorArgPmiSB = 0;

	// is the anchor a noun, verb, adjective, or others
	Symbol anchorPosType = DistributionalUtil::determinePosType(anchorPosTag);
	// is the candidate argument a noun, verb, adjective, or others
        Symbol argPosType = DistributionalUtil::determinePosType(argPosTag);

	// let's assign the scores
	assignSubObjScores(anchor, anchorPosTag, arg, argPosTag);
	assignSimScores(anchor, anchorPosType, arg, argPosType);
	assignPmiScores(anchor, anchorPosType, arg, argPosType);
}

// We assign the <sub> and <obj> scores between (anchor, arg)
// Since we are concerned with <sub> and <obj>, we require the arg to be a noun.
// Also, since our <sub> and <obj> scores are only gathered for nominals, we further require the arg to be a nominal (i.e. not a proper noun) 
//
// populates:
// _subScore , _objScore , _avgSubObjScore , _maxSubObjScore
// _subScoreFea , _subScoreSB , _objScoreFea , _objScoreSB , _avgSubObjScoreFea , _avgSubObjScoreSB , _maxSubObjScoreFea , _maxSubObjScoreSB
void DistributionalAnchorArgClass::assignSubObjScores(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag) {
	//_subScoreFea.clear();
	//_objScoreFea.clear();
	//_avgSubObjScoreFea.clear();
	//_maxSubObjScoreFea.clear();

	_subScore = DistributionalKnowledgeTable::getPredicateSubScore(anchor, arg);
	_objScore = DistributionalKnowledgeTable::getPredicateObjScore(anchor, arg);

	_avgSubObjScore = DistributionalUtil::calculateAvgScore(_subScore, _objScore);
	_maxSubObjScore = std::max(_subScore, _objScore);

	Symbol nominal = Symbol(L"NN");
	Symbol singularNominal = Symbol(L"NNS");

	// check whether the candidate argument is a nominal
	if( (argPosTag==nominal) || (argPosTag==singularNominal) ) {
		//std::vector<std::wstring> bins;
		std::wstring singleBin;

               	_subScoreSB = DistributionalUtil::scoreToBin(_subScore);
               	//bins = DistributionalUtil::scoreToBins(_subScore);
		//for(unsigned i=0; i<bins.size(); i++) {
		//	_subScoreFea.push_back( Symbol(L"subscore_"+bins[i]) );
		//}

               	_objScoreSB = DistributionalUtil::scoreToBin(_objScore);
               	//bins = DistributionalUtil::scoreToBins(_objScore);
		//for(unsigned i=0; i<bins.size(); i++) {
		//	_objScoreFea.push_back( Symbol(L"objscore_"+bins[i]) );
		//}

                _avgSubObjScoreSB = DistributionalUtil::scoreToBin(_avgSubObjScore);
               	//bins = DistributionalUtil::scoreToBins(_avgSubObjScore);
		//for(unsigned i=0; i<bins.size(); i++) {
		//	_avgSubObjScoreFea.push_back( Symbol(L"avgsubobj_"+bins[i]) );
		//}

               	_maxSubObjScoreSB = DistributionalUtil::scoreToBin(_maxSubObjScore);
               	//bins = DistributionalUtil::scoreToBins(_maxSubObjScore);
		//for(unsigned i=0; i<bins.size(); i++) {
		//	_maxSubObjScoreFea.push_back( Symbol(L"maxsubobj_"+bins[i]) );
		//}
	}
	else {
		_subScoreSB = 0;
		_objScoreSB = 0;
		_avgSubObjScoreSB = 0;
		_maxSubObjScoreSB = 0;
	}
}

// We assume arg is always a noun.
// We assume anchor could be a verb or a noun, and although we pass in its pos-type, we do not currently use it.
// Thus, we check both VN and NN scores.
// populates: _anchorArgPmi , _anchorArgPmiFea , _anchorArgPmiSB
void DistributionalAnchorArgClass::assignPmiScores(const Symbol& anchor, const Symbol& anchorPosType, const Symbol& arg, const Symbol& argPosType) {
	float maxS=-1, s=-1;
	//_anchorArgPmiFea.clear();

	s = DistributionalKnowledgeTable::getVNPmi(anchor, arg);
	if(s>maxS)
		maxS = s;
	s = DistributionalKnowledgeTable::getNNPmi(anchor, arg);
	if(s>maxS)
		maxS = s;
	_anchorArgPmi = maxS;

	_anchorArgPmiSB = DistributionalUtil::scoreToBin(_anchorArgPmi);
	//std::vector<std::wstring> bins = DistributionalUtil::scoreToBins(maxS);
	//for(unsigned i=0; i<bins.size(); i++) {
	//	_anchorArgPmiFea.push_back( Symbol(L"aapmi_"+bins[i]) );
	//}
}

// We assume arg is always a noun.
// We assume anchor could be a verb or a noun, and although we pass in its pos-type, we do not currently use it.
// Thus, we check both VN and NN scores.
// populates: _anchorArgSim , _anchorArgSimFea , _anchorArgSimSB
void DistributionalAnchorArgClass::assignSimScores(const Symbol& anchor, const Symbol& anchorPosType, const Symbol& arg, const Symbol& argPosType) {
	float maxS=-1, s=-1;
	//_anchorArgSimFea.clear();

	s = DistributionalKnowledgeTable::getVNSim(anchor, arg);
	if(s>maxS)
		maxS = s;
	s = DistributionalKnowledgeTable::getNNSim(anchor, arg);
	if(s>maxS)
		maxS = s;
	_anchorArgSim = maxS;

        _anchorArgSimSB = DistributionalUtil::scoreToBin(_anchorArgSim);
        //std::vector<std::wstring> bins = DistributionalUtil::scoreToBins(maxS);
	//for(unsigned i=0; i<bins.size(); i++) {
	//	_anchorArgSimFea.push_back( Symbol(L"aasim_"+bins[i]) );
	//}
}

