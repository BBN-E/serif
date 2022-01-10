// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/names/ch_IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/linuxPort/serif_port.h"
#include <wchar.h>
#include <iostream>

ChineseIdFWordFeatures::ChineseIdFWordFeatures(int mode, IdFListSet *listSet) {
	_listSet = listSet;
	
	asciiDigit = Symbol(L":asciiDigit");
	chineseDigit = Symbol(L":chineseDigit");
	asciiPunct = Symbol(L":asciiPunct");
	chinesePunct = Symbol(L":chinesePunct");
	otherPunct = Symbol(L":otherPunct");
	
	isASCII = Symbol(L":isASCII");
	firstWord = Symbol(L":firstWord");
	otherFeatures = Symbol(L":otherFeatures");

	twoDigitNum = Symbol(L":twoDigitNum");
	twoDigitChineseNum = Symbol(L":twoDigitChineseNum");
	fourDigitNum = Symbol(L":fourDigitNum");
	fourDigitChineseNum = Symbol(L":fourDigitChineseNum");
	containsDigitAndAlpha = Symbol(L":containsDigitAndAlpha");
	containsDigitAndDash = Symbol(L":containsDigitAndDash");
	containsDigitAndSlash = Symbol(L":containsDigitAndSlash");
	containsDigitAndComma = Symbol(L":containsDigitAndComma");
	containsDigitAndColon = Symbol(L":containsDigitAndColon");
	containsDigitAndPeriod = Symbol(L":containsDigitAndPeriod");
	containsDigitAndPercent = Symbol(L":containsDigitAndPercent");
	containsDigitAndYear = Symbol(L":containsDigitAndYear");
	containsDigitAndMonth = Symbol(L":containsDigitAndMonth");
	containsDigitAndDay = Symbol(L":containsDigitAndDay");
	otherNum = Symbol(L":otherNum");
}

bool ChineseIdFWordFeatures::containsDigit(Symbol wordSym) const
{
	std::wstring word = wordSym.to_string();
	
	size_t length = word.length();

	for (size_t i = 0; i < length; ++i) {
		if (iswdigit(word[i]) || isChineseDigit(word[i]))
			return true;
	}

	return false;
}

Symbol ChineseIdFWordFeatures::features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null())
			return listFeature;
	}
	
	std::wstring word = wordSym.to_string();
	
	size_t length = word.length();

	if (length == 1) {
		if (iswdigit(word[0]) && iswascii(word[0])) 
			return asciiDigit;
		if (isChineseDigit(word[0]))
			return chineseDigit;
		if (iswpunct(word[0]) && iswascii(word[0]))
			return asciiPunct;
		if (isChinesePunct(word[0]))
			return chinesePunct;
		if (iswpunct(word[0]))
			return otherPunct;
	}
	if (length == 2 && iswdigit(word[0]) && iswdigit(word[1]))
		return twoDigitNum;
	if (length == 2 && isChineseDigit(word[0]) && isChineseDigit(word[1]))
		return twoDigitChineseNum;
	if (length == 4 && iswdigit(word[0]) && iswdigit(word[1]) &&
		iswdigit(word[2]) && iswdigit(word[3]))
		return fourDigitNum;
	if (length == 4 && isChineseDigit(word[0]) && isChineseDigit(word[1]) &&
		isChineseDigit(word[2]) && isChineseDigit(word[3]))
		return fourDigitChineseNum;

	bool all_ascii = true;
	bool contains_digit = false;
	bool contains_dash = false;
	bool contains_comma = false;
	bool contains_colon = false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	bool contains_percent = false;
	bool contains_year = false;
	bool contains_month = false;
	bool contains_day = false;
	for (size_t i = 0; i < length; ++i) {
		if (!iswascii(word[i]))  
            all_ascii = false;
		if (iswdigit(word[i]) || isChineseDigit(word[i]))
			contains_digit = true;
		else if (word[i] == L'.' || word[i] == L'\x3002' || word[i] == L'\xff0e')
			contains_period = true;
		else if (word[i] == L'-' || word[i] == L'\xff0d')
			contains_dash = true;
		else if (word[i] == L',' || word[i] == L'\x3001' || word[i] == L'\xff0c')
			contains_comma = true;
		else if (word[i] == L':' || word[i] == L'\xff1a')
			contains_colon = true;
		else if (word[i] == L'/' || word[i] == L'\xff0f')
			contains_slash = true;
		else if (word[i] == L'%' || word[i] == L'\xff05')
			contains_percent = true;
		else if (word[i] == L'\x5e74')
			contains_year = true;
		else if (word[i] == L'\x6708')
			contains_month = true;
		else if (word[i] == L'\x65e5')
			contains_day = true;
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
		else if (contains_year)
			return containsDigitAndYear;
		else if (contains_month)
			return containsDigitAndMonth;
		else if (contains_day)
			return containsDigitAndDay;
		else return otherNum;
	}
	else if (all_ascii)
		return isASCII;
	else if (first_word) 
		return firstWord;

	return otherFeatures;
	
}

int ChineseIdFWordFeatures::getAllFeatures(Symbol wordSym, bool first_word, 
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
		if (!listFeature.is_null())
			featureArray[num_features++] = listFeature;
	}

	if (num_features >= max_features) return num_features;

	std::wstring word = wordSym.to_string();
	size_t length = word.length();

	if (length == 1) {
		if (iswdigit(word[0]) && iswascii(word[0])) 
			featureArray[num_features++] = asciiDigit;
		else if (isChineseDigit(word[0]))
			featureArray[num_features++] = chineseDigit;
		else if (iswpunct(word[0]) && iswascii(word[0]))
			featureArray[num_features++] = asciiPunct;
		else if (isChinesePunct(word[0]))
			featureArray[num_features++] = chinesePunct;
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
	bool contains_year = false;
	bool contains_month = false;
	bool contains_day = false;
	for (size_t i = 0; i < length; ++i) {
		if (!iswascii(word[i]))  
            all_ascii = false;
		if (iswdigit(word[i]) || isChineseDigit(word[i]))
			contains_digit = true;
		else if (word[i] == L'.' || word[i] == L'\x3002' || word[i] == L'\xff0e')
			contains_period = true;
		else if (word[i] == L'-' || word[i] == L'\xff0d')
			contains_dash = true;
		else if (word[i] == L',' || word[i] == L'\x3001' || word[i] == L'\xff0c')
			contains_comma = true;
		else if (word[i] == L':' || word[i] == L'\xff1a')
			contains_colon = true;
		else if (word[i] == L'/' || word[i] == L'\xff0f')
			contains_slash = true;
		else if (word[i] == L'%' || word[i] == L'\xff05')
			contains_percent = true;
		else if (word[i] == L'\x5e74')
			contains_year = true;
		else if (word[i] == L'\x6708')
			contains_month = true;
		else if (word[i] == L'\x65e5')
			contains_day = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;
	}

	// get number feature
	if (contains_digit){
		if (length == 2 && iswdigit(word[0]) && iswdigit(word[1])) 
			featureArray[num_features++] = twoDigitNum;
		else if (length == 2 && isChineseDigit(word[0]) && isChineseDigit(word[1]))
			featureArray[num_features++] = twoDigitChineseNum;
		else if (length == 4 && iswdigit(word[0]) && iswdigit(word[1]) &&
			iswdigit(word[2]) && iswdigit(word[3]))
			featureArray[num_features++] = fourDigitNum;
		else if (length == 4 && isChineseDigit(word[0]) && isChineseDigit(word[1]) &&
			isChineseDigit(word[2]) && isChineseDigit(word[3]))
			featureArray[num_features++] = fourDigitChineseNum;
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
			if (contains_year)
				featureArray[num_features++] = containsDigitAndYear;
			if (num_features >= max_features) return num_features;
			if (contains_month)
				featureArray[num_features++] = containsDigitAndMonth;
			if (num_features >= max_features) return num_features;
			if (contains_day)
				featureArray[num_features++] = containsDigitAndDay;
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

// borrowed from ch_WordFeatures
bool ChineseIdFWordFeatures::isChineseDigit(wchar_t c_char) const 
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
       (c_char == 0xff19) ||
       (c_char == 0x4ebf) ||
       (c_char == 0x4e07) ||
       (c_char == 0x5343) ||
       (c_char == 0x767e) ||
       (c_char == 0x5341) ||
       (c_char == 0x4e00) ||
       (c_char == 0x4e8c) ||
       (c_char == 0x4e09) ||
       (c_char == 0x56db) ||
       (c_char == 0x4e94) ||
       (c_char == 0x516d) ||
       (c_char == 0x4e03) ||
       (c_char == 0x516b) ||
       (c_char == 0x4e5d))
		return true;
	return false;
}

bool ChineseIdFWordFeatures::isChinesePunct(wchar_t c_char) const {
	if (c_char >= 0x3000 && c_char <= 0x303f)
		return true;
	else
		return false;
}
