// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_IDF_WORD_FEATURES_H
#define ar_IDF_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFWordFeatures.h"
class IdFListSet;


class ArabicIdFWordFeatures : public IdFWordFeatures {
private:
    ArabicIdFWordFeatures(int mode = DEFAULT, IdFListSet *listSet = 0);
	friend struct IdFWordFeatures::FactoryFor<ArabicIdFWordFeatures>;
public:
    Symbol features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const;
	bool containsDigit(Symbol wordSym) const;
	Symbol normalizedWord(Symbol wordSym) const { return wordSym; }




private:
	Symbol otherFeatures;
	Symbol hasAscii;
	Symbol capitalAscii;
	Symbol hasDigit;
	bool isCapitalized(Symbol wordSym) const;
	bool containsASCII(Symbol wordSym) const;
};

#endif
