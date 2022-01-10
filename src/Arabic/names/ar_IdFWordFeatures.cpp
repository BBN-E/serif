// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/names/ar_IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"
#include <wchar.h>
#include <iostream>
#include "Generic/linuxPort/serif_port.h"

ArabicIdFWordFeatures::ArabicIdFWordFeatures(int mode, IdFListSet *listSet)
{
	_listSet = listSet;
	otherFeatures = Symbol(L":otherFeatures");
	hasDigit = Symbol(L":hasDigit");
	hasAscii = Symbol(L":hasAscii");
	capitalAscii = Symbol(L":capitalAscii");
};

Symbol ArabicIdFWordFeatures::features(Symbol wordSym, bool first_word, 
			bool normalized_word_is_in_vocab) const
{
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null())
			return listFeature;
	}

	if(containsDigit(wordSym))
		return hasDigit;
	if(containsASCII(wordSym)){
		if(isCapitalized(wordSym)){
			return capitalAscii;
		}
		else{
			return hasAscii;
		}
	}
	else 
		return otherFeatures;
}
bool ArabicIdFWordFeatures::containsDigit(Symbol wordSym) const{
	std::wstring word = wordSym.to_string();
	size_t length = word.length();
	bool contains_digit = false;
	for (unsigned i = 0; i < length; ++i) {
		if (iswdigit(word[i])){
			contains_digit = true;
			break;
		}
	}
	return contains_digit;
}
bool ArabicIdFWordFeatures::containsASCII(Symbol wordSym) const{
	std::wstring word = wordSym.to_string();
	size_t length = word.length();
	for(unsigned i = 0; i<  length; ++i){
		if(iswascii(word[i])){
			return true;
		}
	}
	return false;
}
bool ArabicIdFWordFeatures::isCapitalized(Symbol wordSym) const{
	std::wstring word = wordSym.to_string();
	if(iswascii(word[0]) && iswupper(word[0])){
		return true;
	}
	else{
		return false;
	}
}
