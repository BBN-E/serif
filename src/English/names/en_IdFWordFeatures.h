// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_IDF_WORD_FEATURES_H
#define en_IDF_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFWordFeatures.h"
class IdFListSet;


class EnglishIdFWordFeatures : public IdFWordFeatures {
private:
    EnglishIdFWordFeatures(int mode = DEFAULT, IdFListSet *listSet = 0);
	friend struct IdFWordFeatures::FactoryFor<EnglishIdFWordFeatures>;
public:
    Symbol features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const;	
	int getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const;	
	bool containsDigit(Symbol wordSym) const;
	Symbol normalizedWord(Symbol wordSym) const;
	void setMode(int mode) { _mode = mode; }
	static Symbol extendedFeatures(Symbol wordSym, bool first_word);

private:
	int _mode;

	Symbol getCaseFeature(std::wstring word, bool first_word, 
		bool normalized_word_is_in_vocab) const;

public:
	static Symbol twoDigitNum;
	static Symbol fourDigitNum;
	static Symbol containsDigitAndAlpha;
	static Symbol containsDigitAndDash;
	static Symbol containsDigitAndSlash;
	static Symbol containsDigitAndComma;
	static Symbol containsDigitAndPeriod;
	static Symbol containsDigitAndPunct;
	static Symbol containsAlphaAndPunct;
	static Symbol containsAtSign;
	static Symbol otherNum;
	static Symbol allCaps;
	static Symbol capPeriod;
	static Symbol firstWordInVocabAsLowercase;
	static Symbol firstWord;
	static Symbol initCap;
	static Symbol lowerCase;
	static Symbol otherFeatures;
	static Symbol punctuation;
	static Symbol bracket;
	static Symbol formulaicTime;

	// for TIMEX
	static Symbol oneToTwelve;
    static Symbol oneToThirtyOne;
    static Symbol zeroToSixty;
    static Symbol otherTwoDigitNum;
    static Symbol yearNum;

private:
	Symbol getTimexFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const;
	Symbol getOtherValueFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const;
	int getAllTimexFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const { return 0; }
	int getAllOtherValueFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const { return 0; }
		
};

#endif





