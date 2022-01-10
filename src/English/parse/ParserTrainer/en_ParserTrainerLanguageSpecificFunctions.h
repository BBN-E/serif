// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define en_PARSERTRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"

#include "Generic/common/Symbol.h"
#include "English/parse/en_STags.h"

class EnglishParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:

	static void initialize();

	static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal){
		if ((label == EnglishSTags::NP) && childIsPreTerminal) {
			if (childLabel == EnglishSTags::POS) {
				return  EnglishSTags::NPPOS;
			}
			else{
				return EnglishSTags::NPA;
			}
		}
		else return label;
	};
	/**
	* Given the label of a HeadlessParseNode  return NPP for name lables,
	* otherwise return the label.  Used by the ParserTrainer
	*
	*/
	static Symbol adjustNameLabelForTraining(Symbol label){
		if ((label == EnglishSTags::PERSON ) ||
			(label == EnglishSTags::ORGANIZATION) ||
			(label == EnglishSTags::LOCATION) ) {
				return EnglishSTags::NPP;
			}
		else return label;

	};
	/**
	* Given the label of a HeadlessParseNode and the label of its parent,
	* return the appropriate label for it.  Used by the ParserTrainer.
	*
	* changed EMB 7/11/2003
	*/
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) {
		if (child_label == EnglishSTags::NNP) {
			if (parent_label == EnglishSTags::NPP)
				return EnglishSTags::NPP_NNP;
			else if (parent_label == EnglishSTags::DATE)
				return EnglishSTags::DATE_NNP;
		}
		else if (child_label == EnglishSTags::NNPS) {
			if (parent_label == EnglishSTags::NPP)
				return EnglishSTags::NPP_NNPS;
			else if (parent_label == EnglishSTags::DATE)
				return EnglishSTags::DATE_NNPS;
		}	
		return child_label;
	};
	static const std::vector<Symbol> &openClassTags() {
		return _openClassTags; }

private:
	static std::vector<Symbol> _openClassTags;
};
#endif
