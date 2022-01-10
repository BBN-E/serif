// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Korean/names/kr_IdFWordFeatures.h"
#include "names/IdFListSet.h"
#include <wchar.h>
#include <iostream>

KoreanIdFWordFeatures::KoreanIdFWordFeatures(int mode, IdFListSet *listSet) {
	_listSet = listSet;
	
	asciiDigit = Symbol(L":asciiDigit");
	koreanDigit = Symbol(L":koreanDigit");
	asciiPunct = Symbol(L":asciiPunct");
	koreanPunct = Symbol(L":koreanPunct");
	otherPunct = Symbol(L":otherPunct");
	
	isASCII = Symbol(L":isASCII");
	firstWord = Symbol(L":firstWord");
	otherFeatures = Symbol(L":otherFeatures");

	twoDigitNum = Symbol(L":twoDigitNum");
	twoDigitKoreanNum = Symbol(L":twoDigitKoreanNum");
	fourDigitNum = Symbol(L":fourDigitNum");
	fourDigitKoreanNum = Symbol(L":fourDigitKoreanNum");
	containsDigitAndAlpha = Symbol(L":containsDigitAndAlpha");
	containsDigitAndDash = Symbol(L":containsDigitAndDash");
	containsDigitAndSlash = Symbol(L":containsDigitAndSlash");
	containsDigitAndComma = Symbol(L":containsDigitAndComma");
	containsDigitAndColon = Symbol(L":containsDigitAndColon");
	containsDigitAndPeriod = Symbol(L":containsDigitAndPeriod");
	containsDigitAndPercent = Symbol(L":containsDigitAndPercent");
	otherNum = Symbol(L":otherNum");
}

bool KoreanIdFWordFeatures::containsDigit(Symbol wordSym) const
{
	std::wstring word = wordSym.to_string();
	
	size_t length = word.length();

	for (size_t i = 0; i < length; ++i) {
		if (iswdigit(word[i]) || isKoreanDigit(word[i]))
			return true;
	}

	return false;
}

Symbol KoreanIdFWordFeatures::features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (listFeature != Symbol())
			return listFeature;
	}
	
	std::wstring word = wordSym.to_string();
	
	size_t length = word.length();

	if (length == 1) {
		if (iswdigit(word[0]) && iswascii(word[0])) 
			return asciiDigit;
		if (isKoreanDigit(word[0]))
			return koreanDigit;
		if (iswpunct(word[0]) && iswascii(word[0]))
			return asciiPunct;
		if (isKoreanPunct(word[0]))
			return koreanPunct;
		if (iswpunct(word[0]))
			return otherPunct;
	}
	if (length == 2 && iswdigit(word[0]) && iswdigit(word[1]))
		return twoDigitNum;
	if (length == 2 && isKoreanDigit(word[0]) && isKoreanDigit(word[1]))
		return twoDigitKoreanNum;
	if (length == 4 && iswdigit(word[0]) && iswdigit(word[1]) &&
		iswdigit(word[2]) && iswdigit(word[3]))
		return fourDigitNum;
	if (length == 4 && isKoreanDigit(word[0]) && isKoreanDigit(word[1]) &&
		isKoreanDigit(word[2]) && isKoreanDigit(word[3]))
		return fourDigitKoreanNum;

	bool all_ascii = true;
	bool contains_digit = false;
	bool contains_dash = false;
	bool contains_comma = false;
	bool contains_colon = false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	bool contains_percent = false;
	for (size_t i = 0; i < length; ++i) {
		if (!iswascii(word[i]))  
            all_ascii = false;
		if (iswdigit(word[i]) || isKoreanDigit(word[i]))
			contains_digit = true;
		else if (word[i] == L'.' || word[i] == L'\xff0e')
			contains_period = true;
		else if (word[i] == L'-' || word[i] == L'\xff0d')
			contains_dash = true;
		else if (word[i] == L',' || word[i] == L'\xff0c')
			contains_comma = true;
		else if (word[i] == L':' || word[i] == L'\xff1a')
			contains_colon = true;
		else if (word[i] == L'/' || word[i] == L'\xff0f')
			contains_slash = true;
		else if (word[i] == L'%' || word[i] == L'\xff05')
			contains_percent = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;

	}

	if (contains_digit) {
		if (contains_alpha)
			return containsDigitAndAlpha;
		else if (contains_dash)
			return containsDigitAndDash;
		else if (contains_slash)
			return containsDigitAndSlash;
		else if (contains_comma)
			return containsDigitAndComma;
		else if (contains_colon)
			return containsDigitAndColon;
		else if (contains_period)
			return containsDigitAndPeriod;
		else if (contains_percent)
			return containsDigitAndPercent;
		else return otherNum;
	}
	else if (all_ascii)
		return isASCII;
	else if (first_word) 
		return firstWord;

	return otherFeatures;
	
}

int KoreanIdFWordFeatures::getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const 
{
	// possible features:
	// 1) list feature
	// 2) single character feature: asciiDigit OR chineseDigit OR
	//								asciiPunct OR chinesePunt OR
	//								otherPunct
	// 3) digit feature: twoDigitNum OR fourDigitNum OR
	//					 twoDigitChineseNum OR fourDigitChineseNum OR
	//					 ANY OF (containsDigitAndAlpha
	//							 containsDigitAndComma
	//							 containsDigitAndDash
	//							 containsDigitAndPeriod
	//							 containsDigitAndSlash) OR
	//					 otherNum
	// 4) isASCII AND/OR firstWord
	// 5) if none of the above, just return otherFeatures	

	int num_features = 0;

	// list feature
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (listFeature != Symbol())
			featureArray[num_features++] = listFeature;
	}

	if (num_features >= max_features) return num_features;

	std::wstring word = wordSym.to_string();
	size_t length = word.length();

	if (length == 1) {
		if (iswdigit(word[0]) && iswascii(word[0])) 
			featureArray[num_features++] = asciiDigit;
		else if (isKoreanDigit(word[0]))
			featureArray[num_features++] = koreanDigit;
		else if (iswpunct(word[0]) && iswascii(word[0]))
			featureArray[num_features++] = asciiPunct;
		else if (isKoreanPunct(word[0]))
			featureArray[num_features++] = koreanPunct;
		else if (iswpunct(word[0]))
			featureArray[num_features++] = otherPunct;
	}

	if (num_features >= max_features) return num_features;

	bool all_ascii = true;
	bool contains_digit = false;
	bool contains_dash = false;
	bool contains_comma = false;
	bool contains_colon = false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	bool contains_percent = false;
	for (size_t i = 0; i < length; ++i) {
		if (!iswascii(word[i]))  
            all_ascii = false;
		if (iswdigit(word[i]) || isKoreanDigit(word[i]))
			contains_digit = true;
		else if (word[i] == L'.' || word[i] == L'\xff0e')
			contains_period = true;
		else if (word[i] == L'-' || word[i] == L'\xff0d')
			contains_dash = true;
		else if (word[i] == L',' || word[i] == L'\xff0c')
			contains_comma = true;
		else if (word[i] == L':' || word[i] == L'\xff1a')
			contains_colon = true;
		else if (word[i] == L'/' || word[i] == L'\xff0f')
			contains_slash = true;
		else if (word[i] == L'%' || word[i] == L'\xff05')
			contains_percent = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;
	}

	// get number feature
	if (contains_digit){
		if (length == 2 && iswdigit(word[0]) && iswdigit(word[1])) 
			featureArray[num_features++] = twoDigitNum;
		else if (length == 2 && isKoreanDigit(word[0]) && isKoreanDigit(word[1]))
			featureArray[num_features++] = twoDigitKoreanNum;
		else if (length == 4 && iswdigit(word[0]) && iswdigit(word[1]) &&
			iswdigit(word[2]) && iswdigit(word[3]))
			featureArray[num_features++] = fourDigitNum;
		else if (length == 4 && isKoreanDigit(word[0]) && isKoreanDigit(word[1]) &&
			isKoreanDigit(word[2]) && isKoreanDigit(word[3]))
			featureArray[num_features++] = fourDigitKoreanNum;
		else if (!contains_alpha && !contains_dash &&
				!contains_slash && !contains_comma &&
				!contains_period)
		{
				featureArray[num_features++] = otherNum;
		} else {
			if (contains_alpha)
				featureArray[num_features++] = containsDigitAndAlpha;
			if (num_features >= max_features) return num_features;
			if (contains_dash)
				featureArray[num_features++] = containsDigitAndDash;
			if (num_features >= max_features) return num_features;
			if (contains_slash)
				featureArray[num_features++] = containsDigitAndSlash;
			if (num_features >= max_features) return num_features;
			if (contains_comma)
				featureArray[num_features++] = containsDigitAndComma;
			if (contains_colon)
				featureArray[num_features++] = containsDigitAndColon;
			if (num_features >= max_features) return num_features;
			if (contains_period)
				featureArray[num_features++] = containsDigitAndPeriod;
			if (num_features >= max_features) return num_features;
			if (contains_percent)
				featureArray[num_features++] = containsDigitAndPercent;
			if (num_features >= max_features) return num_features;
		}
	}
	if (num_features >= max_features) return num_features;

	if (all_ascii)
		featureArray[num_features++] = isASCII;
	if (num_features >= max_features) return num_features;
	if (first_word)
		featureArray[num_features++] = firstWord;
	if (num_features >= max_features) return num_features;

	if (num_features == 0)
		featureArray[num_features++] = otherFeatures;
		
	/*std::cerr << wordSym.to_debug_string() << ": ";
	for (int i = 0; i < num_features; i++) {
		std::cerr << featureArray[i].to_debug_string() << " ";
	}
	std::cerr << "\n";*/

	return num_features;
}

// really just fullwidth digits for now
bool KoreanIdFWordFeatures::isKoreanDigit(wchar_t c_char) const 
{
	if((c_char == 0xff10) ||
       (c_char == 0xff11) ||
       (c_char == 0xff12) ||
       (c_char == 0xff13) ||
       (c_char == 0xff14) ||
       (c_char == 0xff15) ||
       (c_char == 0xff16) ||
       (c_char == 0xff17) ||
       (c_char == 0xff18) ||
       (c_char == 0xff19))
		return true;
	return false;
}

bool KoreanIdFWordFeatures::isKoreanPunct(wchar_t c_char) const {
	return false;
}




