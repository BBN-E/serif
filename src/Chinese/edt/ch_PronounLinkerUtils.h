// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PRONOUNLINKERUTILS_H
#define CH_PRONOUNLINKERUTILS_H

#include "Generic/edt/PronounLinkerUtils.h"

class ChinesePronounLinkerUtils {
	// Note: this class is intentionally not a subclass of PronounLinkerUtils.
	// See PronounLinkerUtils.h for an explanation.
public:
	static Symbol combineSymbols(Symbol symArray[], int nSymArray, bool use_delimiter = true);
	static Symbol getNormDistSymbol(int distance);
	static Symbol getAugmentedParentHeadWord(const SynNode *node);
	static Symbol getAugmentedParentHeadTag(const SynNode *node);
	static Symbol augmentIfPOS(Symbol type, const SynNode *node);
};

#endif
