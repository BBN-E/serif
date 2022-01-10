// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_SYMBOL_UTILITIES_H
#define AR_SYMBOL_UTILITIES_H

#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/xx_SymbolUtilities.h"

class ArabicSymbolUtilities : public GenericSymbolUtilities {
public:
	static Symbol stemDescriptor(Symbol word);
	static Symbol getFullNameSymbol(const SynNode *nameNode);
	static Symbol getStemmedFullName(const SynNode *nameNode);
	static int getStemVariants(Symbol word, Symbol* variants, int MAX_RESULTS);
	static Symbol getWordWithoutAl(Symbol word);

	/** Definitions for the following methods are currently inherited from
	 * GenericSymbolUtilities: */
	//static int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS);
	//static int fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS);
	//static int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS);
	//static float getDescAdjScore(MentionSet* mentionSet);
	//static Symbol lowercaseSymbol(Symbol sym);
	//static Symbol stemWord(Symbol word, Symbol pos);
	//static bool isPluralMention(const Mention* ment);
	//static bool isBracket(Symbol word);
	//static bool isClusterableWord(Symbol word);
};

#endif
