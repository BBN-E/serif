// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/parse/ParserTrainer/ch_ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "Chinese/parse/ch_STags.h"

std::vector<Symbol> ChineseParserTrainerLanguageSpecificFunctions::_openClassTags;

void ChineseParserTrainerLanguageSpecificFunctions::initialize(){
	if (!_openClassTags.empty()) return; // already initialized
	_openClassTags.push_back(ChineseSTags::VA); 
	_openClassTags.push_back(ChineseSTags::VV); 
	_openClassTags.push_back(ChineseSTags::NR); 
	_openClassTags.push_back(ChineseSTags::NT);
	_openClassTags.push_back(ChineseSTags::NN);
	_openClassTags.push_back(ChineseSTags::CD);
	_openClassTags.push_back(ChineseSTags::OD);
	_openClassTags.push_back(ChineseSTags::M);
	_openClassTags.push_back(ChineseSTags::AD);
	_openClassTags.push_back(ChineseSTags::IJ);
	_openClassTags.push_back(ChineseSTags::ON);
	_openClassTags.push_back(ChineseSTags::JJ);
	_openClassTags.push_back(ChineseSTags::FW);
};
