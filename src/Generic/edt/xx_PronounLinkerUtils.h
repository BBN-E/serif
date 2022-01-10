// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_PRONOUNLINKERUTILS_H
#define XX_PRONOUNLINKERUTILS_H

#include "Generic/edt/PronounLinkerUtils.h"

class GenericPronounLinkerUtils {
	// Note: this class is intentionally not a subclass of PronounLinkerUtils.
	// See PronounLinkerUtils.h for an explanation.
public:
	static Symbol combineSymbols(Symbol symArray[], int nSymArray, 
					             bool use_delimiter = true)
	{
		return Symbol(L"");
	}
	static Symbol getNormDistSymbol(int distance) { return Symbol(L""); }
	static Symbol getAugmentedParentHeadWord(const SynNode *node) { return Symbol(L""); }
	static Symbol getAugmentedParentHeadTag(const SynNode *node)  { return Symbol(L""); }
	static Symbol augmentIfPOS(Symbol type, const SynNode *node)  { return Symbol(L""); }
};

#endif
