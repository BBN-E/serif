// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/ParserTrainer/es_ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)

std::vector<Symbol> SpanishParserTrainerLanguageSpecificFunctions::_openClassTags;

namespace {
	static boost::wregex STRIP_MODIFIERS_RE;
}

void SpanishParserTrainerLanguageSpecificFunctions::initialize(){
	if (!_openClassTags.empty()) return; // already initialized
	// adjectives, nouns, adverbs, verbs, dates, numbers
	Symbol::SymbolGroup tags = Symbol::makeSymbolGroup
		(L"aq nc np rg rn vag vai vam van vap vas vmg vmi vmm vmn vmp vms vsg vsi vsm vsn vsp vss w z zc");
	_openClassTags.insert(_openClassTags.begin(), tags.begin(), tags.end());

	// Assemble the modifier stripping RE
	std::wstring strip_modifiers_re;
	strip_modifiers_re += L"(\\.[mfc]?[spc]?)?(\\.co)?";
	if (ParamReader::isInitialized() && ParamReader::isParamTrue("parser_strip_subord_modifier"))
		strip_modifiers_re += L"(\\.subord)?";
	strip_modifiers_re += L"(\\.x)?(\\.j)?$";
	STRIP_MODIFIERS_RE = boost::wregex(strip_modifiers_re);
};


Symbol SpanishParserTrainerLanguageSpecificFunctions::adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal){
	std::wstring labelStr(label.to_string());
	//std::cerr << labelStr << "->";
	labelStr = boost::regex_replace(labelStr, STRIP_MODIFIERS_RE, L"");
	//std::cerr << labelStr << std::endl;
	label = Symbol(labelStr.c_str());

	// Mark NP nodes where child is a preterminal as NPA.
	if ((label == SpanishSTags::NP) && childIsPreTerminal) {
		return SpanishSTags::NPA;
	}
	return label;
};

Symbol  SpanishParserTrainerLanguageSpecificFunctions::makeNPDefLabelsForTraining(Symbol label, Symbol headPretermLabel){
	// if(LanguageSpecificFunctions::isNPtypeLabel(label)){
	// 	if(SpanishSTags::isNoun(headPretermLabel)){
	// 		if(label == SpanishSTags::NPA){
	// 			return SpanishSTags::DEF_NPA;
	// 		}
	// 		else if(label == SpanishSTags::NP){
	// 			return SpanishSTags::DEF_NP;
	// 		}
	// 	}
	// }
	return label;
}
