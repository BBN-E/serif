// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define es_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"

class SpanishParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:

	// We overload this to do some things that weren't originally
	// intended, such as stripping off gender/number.
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
