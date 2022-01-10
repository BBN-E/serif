// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "English/parse/en_STags.h"

std::vector<Symbol> EnglishParserTrainerLanguageSpecificFunctions::_openClassTags;

void EnglishParserTrainerLanguageSpecificFunctions::initialize(){
	if (!_openClassTags.empty()) return; // already initialized
	_openClassTags.push_back(EnglishSTags::CD);
	_openClassTags.push_back(EnglishSTags::FW);
	_openClassTags.push_back(EnglishSTags::JJ); 
	_openClassTags.push_back(EnglishSTags::JJR);
	_openClassTags.push_back(EnglishSTags::JJS);
	_openClassTags.push_back(EnglishSTags::LS);
	_openClassTags.push_back(EnglishSTags::NN);
	_openClassTags.push_back(EnglishSTags::NNS);
	_openClassTags.push_back(EnglishSTags::NNP);
	_openClassTags.push_back(EnglishSTags::NNPS);
	_openClassTags.push_back(EnglishSTags::PDT);
	_openClassTags.push_back(EnglishSTags::RB);
	_openClassTags.push_back(EnglishSTags::RBR);
	_openClassTags.push_back(EnglishSTags::RBS);
	_openClassTags.push_back(EnglishSTags::RP);
	_openClassTags.push_back(EnglishSTags::UH);
	_openClassTags.push_back(EnglishSTags::VB);
	_openClassTags.push_back(EnglishSTags::VBD);
	_openClassTags.push_back(EnglishSTags::VBG);
	_openClassTags.push_back(EnglishSTags::VBN);
	_openClassTags.push_back(EnglishSTags::VBP);
	_openClassTags.push_back(EnglishSTags::VBZ);
	_openClassTags.push_back(EnglishSTags::LOCATION_NNP);
	_openClassTags.push_back(EnglishSTags::PERSON_NNP);
	_openClassTags.push_back(EnglishSTags::ORGANIZATION_NNP);
	_openClassTags.push_back(EnglishSTags::PERCENT_NNP);
	_openClassTags.push_back(EnglishSTags::TIME_NNP);
	_openClassTags.push_back(EnglishSTags::DATE_NNP);
	_openClassTags.push_back(EnglishSTags::MONEY_NNP);
	_openClassTags.push_back(EnglishSTags::LOCATION_NNPS);
	_openClassTags.push_back(EnglishSTags::PERSON_NNPS);
	_openClassTags.push_back(EnglishSTags::ORGANIZATION_NNPS);
	_openClassTags.push_back(EnglishSTags::PERCENT_NNPS);
	_openClassTags.push_back(EnglishSTags::TIME_NNPS);
	_openClassTags.push_back(EnglishSTags::DATE_NNPS);
	_openClassTags.push_back(EnglishSTags::MONEY_NNPS);
	_openClassTags.push_back(EnglishSTags::NPP_NNP);
	_openClassTags.push_back(EnglishSTags::NPP_NNPS);
};
