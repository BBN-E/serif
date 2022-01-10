// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ur_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define ur_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/common/Symbol.h"

class UrduParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:

	// We overload this to do some things that weren't originally
	// intended, such as stripping off gender/number.
  static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal) {return label;}

  static Symbol makeNPDefLabelsForTraining(Symbol label, Symbol headPretermLabel) {return label;}

	/**
	* Given the label of a HeadlessParseNode return NPP for name labels,
	* otherwise return the label.  Used by the ParserTrainer.
	*/
	static Symbol adjustNameLabelForTraining(Symbol label){return label;}
	/**
     * Remove the 'HEAD' part of the label that was originally applied
     * in favor of the Headify-applied 'HEAD'
	*/
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label);
		
	static void initialize();

	static const std::vector<Symbol> &openClassTags() {
		return _openClassTags; }

 private:
	static std::vector<Symbol> _openClassTags;
};
#endif
