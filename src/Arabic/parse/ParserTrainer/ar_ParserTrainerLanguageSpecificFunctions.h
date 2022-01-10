// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define ar_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"
#include "Arabic/parse/ar_STags.h"

class ArabicParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:
	/**
	* Given the label of a HeadlessParseNode, the label of its child, if the child is 
	* PreTerminal, return what the new label should be.  Used by the ParserTrainer
	*
	*/
	static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal);

	static Symbol makeNPDefLabelsForTraining(Symbol label, Symbol headPretermLabel);

	/**
	* Given the label of a HeadlessParseNode return NPP for name labels,
	* otherwise return the label.  Used by the ParserTrainer.
	*/
	static Symbol adjustNameLabelForTraining(Symbol label){return label;}
	/**
	* Given the label of a HeadlessParseNode and the label of its parent,
	* return the appropriate label for it.  Used by the ParserTrainer.
	*/
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) 
		{return child_label;}
		
	static void initialize();

	static const std::vector<Symbol> &openClassTags() {
		return _openClassTags; }

 private:
	static std::vector<Symbol> _openClassTags;
};
#endif
