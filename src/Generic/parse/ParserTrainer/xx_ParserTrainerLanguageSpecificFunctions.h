// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_PARSER_TRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define XX_PARSER_TRAINER_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"

class GenericParserTrainerLanguageSpecificFunctions {
	// Note: this class is intentionally not a subclass of
	// ParserTrainerLangaugeSpecificFunctions.  See
	// ParserTrainerLangaugeSpecificFunctions.h for an explanation.
	public:

	static void initialize() {}
	static Symbol adjustNPLabelsForTraining(Symbol label, Symbol childLabel, bool childIsPreTerminal) {
		return SymbolConstants::nullSymbol; 
	}
	static Symbol adjustNameLabelForTraining(Symbol label) { 
		return SymbolConstants::nullSymbol; 
	}
	static Symbol adjustNameChildrenLabelForTraining(Symbol parent_label, Symbol child_label) {
		return SymbolConstants::nullSymbol; 
	}
	static const std::vector<Symbol> &openClassTags() {
		return _openClassTags; }

 private:
	static std::vector<Symbol> _openClassTags;
};


#endif

