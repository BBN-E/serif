// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define ch_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"

class ChineseParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:
	static void initialize();
	static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal){
		if ((label == ChineseSTags::NP) && childIsPreTerminal) {
			return ChineseSTags::NPA;
		}
		
		return label;
	};

	/**
	* Given the label of a HeadlessParseNode return NPP for name labels,
	* otherwise return the label.  Used by the ParserTrainer.
	*/
	static Symbol adjustNameLabelForTraining(Symbol label){return label;}
	/**
	* Given the label of a HeadlessParseNode and the label of its parent,
	* return the appropriate label for it.  Used by the ParserTrainer.
	*/
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, 
		Symbol child_label) {return child_label;}

	static const std::vector<Symbol> &openClassTags() {
		return _openClassTags; }

 private:
	static std::vector<Symbol> _openClassTags;
};
#endif
