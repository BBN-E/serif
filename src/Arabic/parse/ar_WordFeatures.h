// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_WORD_FEATURES_H
#define ar_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include "Generic/parse/WordFeatures.h"

using namespace std;

#define WORD_FEATURES_NUM_PREF 7
#define WORD_FEATURES_NUM_SUFF 11


class ArabicWordFeatures : public WordFeatures {
private:
	friend class ArabicWordFeaturesFactory;

public:
    Symbol features(Symbol wordSym, bool firstWord) const;
    Symbol reducedFeatures(Symbol wordSym, bool firstWord) const;
private:
    ArabicWordFeatures();
	static const int _numPrefix;
	static const int _numSuffix;
	std::wstring _prefix[WORD_FEATURES_NUM_PREF];
	std::wstring _suffix[WORD_FEATURES_NUM_SUFF];
	bool isNumber(const std::wstring& word) const;
	std::wstring prefixFeature(const std::wstring& word)const;
	std::wstring suffixFeature(const std::wstring& word) const;
};

class ArabicWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new ArabicWordFeatures(); }
};

#endif
