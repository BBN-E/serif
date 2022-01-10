#ifndef CLUSTER_WORD_FEATURES_H
#define CLUSTER_WORD_FEATURES_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

#include <string>

using namespace std;

class ClusterWordFeatures
{
public:
	ClusterWordFeatures();
	~ClusterWordFeatures();

	static int lookup(wstring word, bool startOfSentence);

private:
	static int getCaseFeature(wstring word, bool startOfSentence);
	static int getNumericFeature(wstring word);

	static bool twoDigitNum(wstring str);
	static bool fourDigitNum(wstring str);
	static bool allCaps(wstring str);
	static bool capPeriod(wstring str);
	static bool initCap(wstring str);
	static bool allLower(wstring str);
	static bool containsDigit(wstring str);
	static bool containsAlpha(wstring str);
	static bool containsDash(wstring str);
	static bool containsSlash(wstring str);
	static bool containsComma(wstring str);
	static bool containsPeriod(wstring str);

};


#endif
