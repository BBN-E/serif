// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Urdu/parse/ParserTrainer/ur_ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Urdu/parse/ur_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include <boost/regex.hpp>

std::vector<Symbol> UrduParserTrainerLanguageSpecificFunctions::_openClassTags;

namespace {
	static boost::wregex STRIP_MODIFIERS_RE;
}

void UrduParserTrainerLanguageSpecificFunctions::initialize(){
	if (!_openClassTags.empty()) return; // already initialized
	// adjectives, nouns, adverbs, verbs, dates, numbers
	Symbol::SymbolGroup tags = Symbol::makeSymbolGroup
      (L"NN NNC VM PRP JJ NST NNP");
	_openClassTags.insert(_openClassTags.begin(), tags.begin(), tags.end());
};

Symbol UrduParserTrainerLanguageSpecificFunctions::adjustNameChildrenLabelForTraining(Symbol label, Symbol child_label) {
  std::wstring labelStr(child_label.to_string());
  labelStr = boost::regex_replace(labelStr, boost::wregex(L"HEAD-"), L"");
  return labelStr;
};

