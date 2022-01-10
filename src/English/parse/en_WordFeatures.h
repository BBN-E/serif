// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_WORD_FEATURES_H
#define en_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include "Generic/parse/WordFeatures.h"

#define WORD_FEATURES_NUM_DERIV_FEATURES 47


class EnglishWordFeatures : public WordFeatures {
private:
	friend class EnglishWordFeaturesFactory;
public:
    Symbol features(Symbol wordSym, bool firstWord) const;
    Symbol reducedFeatures(Symbol wordSym, bool firstWord) const;
private:
    EnglishWordFeatures();
    static const unsigned numDerivationalFeatures;
    bool isConsonant(wchar_t ch) const;
    bool sInflection(const std::wstring& word) const;
	std::wstring capitalizationFeature(const std::wstring& word, bool firstWord) const;
	std::wstring hyphenizationFeature(const std::wstring& word) const;
	std::wstring inflectionalFeature(const std::wstring& word) const;
	std::wstring derivationalFeature(const std::wstring& word) const;
    bool isNumber(const std::wstring& word) const;
	std::wstring derivationalFeatures[WORD_FEATURES_NUM_DERIV_FEATURES];
    bool consonants[256];
};

class EnglishWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new EnglishWordFeatures(); }
};


#endif





