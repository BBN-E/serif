// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_SYMBOL_UTILITIES
#define XX_SYMBOL_UTILITIES

#include "Generic/common/SymbolUtilities.h"

class GenericSymbolUtilities {
	// Note: this class is intentionally not a subclass of
	// SymbolUtilities.  See SymbolUtilities.h for an explanation.
public:
	// See SymbolUtilities.h for method documentation.
	static Symbol stemDescriptor(Symbol word) {
		return word; }
	static int fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS){
		return 0; }
	static int fillWordNetOffsets(Symbol word, Symbol pos, 
								   int *offsets, int MAX_OFFSETS){
		return 0; }
	static int getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) {
		return 0; }
	static Symbol getFullNameSymbol(const SynNode *nameNode);
	static float getDescAdjScore(MentionSet* mentionSet);
	static Symbol lowercaseSymbol(Symbol sym) {
		return sym; }
	static Symbol stemWord(Symbol word, Symbol pos) {
		return word; }
	static int getStemVariants(Symbol word, Symbol* variants, int MAX_RESULTS){
		return 0; }
	static Symbol getStemmedFullName(const SynNode *nameNode) {
		return getFullNameSymbol(nameNode); }
	static Symbol getWordWithoutAl(Symbol word) {
		return word; }
	static bool isPluralMention(const Mention* ment) {
		return false; }
	static bool isBracket(Symbol word);
	static bool isClusterableWord(Symbol word);

	// The following method isn't defined in SymbolUtilities -- should it be?
	//static int fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS);

};

#endif
