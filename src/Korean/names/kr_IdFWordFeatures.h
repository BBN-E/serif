// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef kr_IDF_WORD_FEATURES_H
#define kr_IDF_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "common/Symbol.h"
#include "names/IdFListSet.h"
#include "Generic/names/IdFWordFeatures.h"

class KoreanIdFWordFeatures : public IdFWordFeatures {
private:
    KoreanIdFWordFeatures(int mode = DEFAULT, IdFListSet *listSet = 0);
	friend struct IdFWordFeatures::FactoryFor<KoreanIdFWordFeatures>;
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
	Symbol koreanDigit;
	Symbol asciiPunct;
	Symbol koreanPunct;
	Symbol otherPunct;
	
	// general features
	Symbol isASCII;
	Symbol firstWord;
	Symbol otherFeatures;

	// multi-character word features
	Symbol twoDigitNum;
	Symbol twoDigitKoreanNum;
	Symbol fourDigitNum;
	Symbol fourDigitKoreanNum;
	Symbol containsDigitAndAlpha;
	Symbol containsDigitAndDash;
	Symbol containsDigitAndSlash;
	Symbol containsDigitAndComma;
	Symbol containsDigitAndColon;
	Symbol containsDigitAndPeriod;
	Symbol containsDigitAndPercent;
	Symbol otherNum;

	bool isKoreanDigit(wchar_t c_char) const; 
	bool isKoreanPunct(wchar_t c_char) const;

};

#endif





