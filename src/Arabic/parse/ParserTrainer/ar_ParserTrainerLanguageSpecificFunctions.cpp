// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ParserTrainer/ar_ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"

std::vector<Symbol> ArabicParserTrainerLanguageSpecificFunctions::_openClassTags;

void ArabicParserTrainerLanguageSpecificFunctions::initialize(){
	if (!_openClassTags.empty()) return; // already initialized
	_openClassTags.push_back(ArabicSTags::DET_JJ); 
	_openClassTags.push_back(ArabicSTags::JJ);
	_openClassTags.push_back(ArabicSTags::DET_NN);
	_openClassTags.push_back(ArabicSTags::NN);
	_openClassTags.push_back(ArabicSTags::DET_NNS);
	_openClassTags.push_back(ArabicSTags::NNS);
	_openClassTags.push_back(ArabicSTags::DET_NNP);
	_openClassTags.push_back(ArabicSTags::NNP);
	_openClassTags.push_back(ArabicSTags::DET_NNPS);
	_openClassTags.push_back(ArabicSTags::NNPS);
	_openClassTags.push_back(ArabicSTags::RB);
	_openClassTags.push_back(ArabicSTags::VB);
	_openClassTags.push_back(ArabicSTags::VBD);
	_openClassTags.push_back(ArabicSTags::VBN);
	_openClassTags.push_back(ArabicSTags::VBP);
	_openClassTags.push_back(ArabicSTags::CD);
/*	//used in ArabicParser experiments but are not currently used in our system
	_openClassTags.push_back(ArabicSTags::DV);
	_openClassTags.push_back(ArabicSTags::JJ_NUM);
	_openClassTags.push_back(ArabicSTags::JJ_COMP);
	_openClassTags.push_back(ArabicSTags::NQ);
	_openClassTags.push_back(ArabicSTags::DET_JJ_NUM);
	_openClassTags.push_back(ArabicSTags::DET_JJ_COMP);
	_openClassTags.push_back(ArabicSTags::DET_NQ);
*/
};


Symbol ArabicParserTrainerLanguageSpecificFunctions::adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal){
	Symbol test = ArabicSTags::NP;
		if ((label == ArabicSTags::NP) && childIsPreTerminal) {
			return ArabicSTags::NPA;
		}
		else return label;
	};

Symbol  ArabicParserTrainerLanguageSpecificFunctions::makeNPDefLabelsForTraining(Symbol label, Symbol headPretermLabel){
	if(LanguageSpecificFunctions::isNPtypeLabel(label)){
		if(ArabicSTags::isNoun(headPretermLabel)){
			if(label == ArabicSTags::NPA){
				return ArabicSTags::DEF_NPA;
			}
			else if(label == ArabicSTags::NP){
				return ArabicSTags::DEF_NP;
			}
		}
	}
	return label;

}
