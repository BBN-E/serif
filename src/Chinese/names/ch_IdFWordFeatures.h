// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_IDF_WORD_FEATURES_H
#define ch_IDF_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"

class ChineseIdFWordFeatures : public IdFWordFeatures {
private:
    ChineseIdFWordFeatures(int mode = DEFAULT, IdFListSet *listSet = 0);
	friend struct IdFWordFeatures::FactoryFor<ChineseIdFWordFeatures>;
public:
    Symbol features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const;	
	int getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const;	
	bool containsDigit(Symbol wordSym) const;
	Symbol normalizedWord(Symbol wordSym) const { return wordSym; }

private:
	// single character features
	Symbol asciiDigit;
	Symbol chineseDigit;
	Symbol asciiPunct;
	Symbol chinesePunct;
	Symbol otherPunct;
	
	// general features
	Symbol isASCII;
	Symbol firstWord;
	Symbol otherFeatures;

	// multi-character word features
	Symbol twoDigitNum;
	Symbol twoDigitChineseNum;
	Symbol fourDigitNum;
	Symbol fourDigitChineseNum;
	Symbol containsDigitAndAlpha;
	Symbol containsDigitAndDash;
	Symbol containsDigitAndSlash;
	Symbol containsDigitAndComma;
	Symbol containsDigitAndColon;
	Symbol containsDigitAndPeriod;
	Symbol containsDigitAndPercent;
	Symbol containsDigitAndYear;
	Symbol containsDigitAndMonth;
	Symbol containsDigitAndDay;
	Symbol otherNum;

	bool isChineseDigit(wchar_t c_char) const; 
	bool isChinesePunct(wchar_t c_char) const;

};

#endif
