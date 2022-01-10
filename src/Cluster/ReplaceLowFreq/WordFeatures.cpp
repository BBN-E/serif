// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.



#include "Cluster/ReplaceLowFreq/WordFeatures.h"
#include <wctype.h>

ClusterWordFeatures::ClusterWordFeatures()
{
}

ClusterWordFeatures::~ClusterWordFeatures()
{
}

static int _twoDigitNum = 1;
static int _fourDigitNum = 2;
static int _containsDigitAndAlpha = 3;
static int _containsDigitAndDash = 4;
static int _containsDigitAndSlash = 5;
static int _containsDigitAndComma = 6;
static int _containsDigitAndPeriod = 7;
static int _otherNum = 8;
static int _allCaps = 9;
static int _capPeriod = 10;
static int _firstWord = 11;
static int _initCap = 12;
static int _lowerCase = 13;

int ClusterWordFeatures::lookup(wstring word, bool startOfSentence) {
    int numericFeature = getNumericFeature(word);
    if (numericFeature != 0) {
      return numericFeature;
    }
    int caseFeature = getCaseFeature(word, startOfSentence);
    if (caseFeature != 0) {
      return caseFeature;
    }
    return 0;
}

int ClusterWordFeatures::getCaseFeature(wstring word, bool startOfSentence) {
    if (allCaps(word)) {
      return _allCaps;
    }
    if (capPeriod(word)) {
      return _capPeriod;
    }
    if (initCap(word)) {
      if (startOfSentence) {
        return _firstWord;
      }
      else {
        return _initCap;
      }
    }
    if (allLower(word)) {
      return _lowerCase;
    }
    return 0;
}

int ClusterWordFeatures::getNumericFeature(wstring word) {
    if (twoDigitNum(word)) {
      return _twoDigitNum;
    }
    if (fourDigitNum(word)) {
      return _fourDigitNum;
    }
    if (containsDigit(word)) {
      if (containsAlpha(word)) {
        return _containsDigitAndAlpha;
      }
      else if (containsDash(word)) {
        return _containsDigitAndDash;
      }
      else if (containsSlash(word)) {
        return _containsDigitAndSlash;
      }
      else if (containsComma(word)) {
        return _containsDigitAndComma;
      }
      else if (containsPeriod(word)) {
        return _containsDigitAndPeriod;
      }
      else {
        return _otherNum;
      }
    }
    return 0;
}


bool ClusterWordFeatures::twoDigitNum(wstring str) {
	 if ((str.length() > 2) || (str.length() < 2))
		return false;

	 const wchar_t * wchars = str.c_str();
	 for (size_t i = 0; i < str.length(); i++) {
		if (!iswdigit(wchars[i]))
			return false;
	 }
	 return true;
}

bool ClusterWordFeatures::fourDigitNum(wstring str) {
	 if ((str.length() > 4) || (str.length() < 4))
		return false;

	 const wchar_t * wchars = str.c_str();
	 for (size_t i = 0; i < str.length(); i++) {
		if (!iswdigit(wchars[i]))
			return false;
	 }
	 return true;
  }

bool ClusterWordFeatures::allCaps(wstring str) {
	 const wchar_t * wchars = str.c_str();
	 for (size_t i = 0; i < str.length(); i++) {
		if (!iswupper(wchars[i]))
			return false;
	 }
	 return true;
  }

bool ClusterWordFeatures::capPeriod(wstring str) {
	if ((str.length() > 2) || (str.length() < 2))
		return false;
	 const wchar_t * wchars = str.c_str();
	 return (iswupper(wchars[0]) && (wchars[1] == L'.'));
}

bool ClusterWordFeatures::initCap(wstring str) {
	const wchar_t * wchars = str.c_str();
	return ((str.length() > 0) && (iswupper(wchars[0])));
}

bool ClusterWordFeatures::allLower(wstring str) {
	 const wchar_t * wchars = str.c_str();
	 for (size_t i = 0; i < str.length(); i++) {
		if (!iswlower(wchars[i]))
			return false;
	 }
	 return true;
}

bool ClusterWordFeatures::containsDigit(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (iswdigit(wchars[i]))
			return true;
	}
	return false;
}

bool ClusterWordFeatures::containsAlpha(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (iswalpha(wchars[i]))
			return true;
	}
	return false;
}

bool ClusterWordFeatures::containsDash(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (wchars[i] == L'-')
			return true;
	}
	return false;
}

bool ClusterWordFeatures::containsSlash(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (wchars[i] == L'/')
			return true;
	}
	return false;
}

bool ClusterWordFeatures::containsComma(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (wchars[i] == L',')
			return true;
	}
	return false;
}

bool ClusterWordFeatures::containsPeriod(wstring str) {
	const wchar_t * wchars = str.c_str();
	for (size_t i = 0; i < str.length(); i++) {
		if (wchars[i] == L'.')
			return true;
	}
	return false;
}
