// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SYMBOL_UTILITIES_H
#define EN_SYMBOL_UTILITIES_H

#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/xx_SymbolUtilities.h"

class EnglishSymbolUtilities: public GenericSymbolUtilities {
public:
	static Symbol stemDescriptor(Symbol word);
	static int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS);
	static int fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS);
	static int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS);
	static Symbol getFullNameSymbol(const SynNode *nameNode);
	static float getDescAdjScore(MentionSet* mentionSet);
	static Symbol lowercaseSymbol(Symbol sym);
	static Symbol stemWord(Symbol word, Symbol pos);
	static Symbol getWordWithoutAl(Symbol word);
	static bool isPluralMention(const Mention* ment);

	/** Definitions for the following methods are currently inherited from
	 * GenericSymbolUtilities: */
	//static bool isBracket(Symbol word);
	//static bool isClusterableWord(Symbol word);
	//static int getStemVariants(Symbol word, Symbol* variants, int MAX_RESULTS);
	//static Symbol getStemmedFullName(const SynNode *nameNode);

};


#endif
