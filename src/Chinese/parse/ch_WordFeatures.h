// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_WORD_FEATURES_H
#define ch_WORD_FEATURES_H

//#include <string>
//#include <cstdio>
#include "Generic/parse/WordFeatures.h"
#include "Generic/common/Symbol.h"

#define WORD_FEATURES_NUM_DERIV_FEATURES 190


class ChineseWordFeatures : public WordFeatures {
private:
	friend class ChineseWordFeaturesFactory;
public:
    Symbol features(Symbol wordSym, bool firstWord) const;
    Symbol reducedFeatures(Symbol wordSym, bool firstWord) const;
private:
    ChineseWordFeatures();
    static const unsigned numDerivationalFeatures;
	std::wstring derivationalFeature(const std::wstring& word) const;
    bool isEnglishNumber(const std::wstring& word) const;
	bool isChineseNumber(const std::wstring& word) const;
	bool isChineseNumChar(wchar_t c_char) const;
	bool isChineseDigit(wchar_t c_char) const;
	bool isChinese(const std::wstring& word) const;
	wchar_t derivationalFeatures[WORD_FEATURES_NUM_DERIV_FEATURES];

};

class ChineseWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new ChineseWordFeatures(); }
};


#endif





