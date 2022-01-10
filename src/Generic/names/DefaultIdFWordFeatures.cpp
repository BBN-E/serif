// Copyright 2011 BBN Technologies
// All Rights Reserved

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/names/DefaultIdFWordFeatures.h"
#include "Generic/common/SymbolUtilities.h"

namespace {
	Symbol otherFeatures(L":otherFeatures");
	Symbol firstWord(L":firstWord");
	Symbol firstWordInVocabAsLowercase(L":firstWordInVocabAsLowercase");
	Symbol initCap(L":initCap");
	Symbol capPeriod(L":capPeriod");
	Symbol allCaps(L":allCaps");
	Symbol bracket(L":bracket");
	Symbol number(L":number");
	Symbol otherNum(L":otherNum");
	Symbol containsAscii(L":containsAscii");
	Symbol containsDigitAndAlpha(L":containsDigitAndAlpha");
	Symbol containsDigitAndDash(L":containsDigitAndDash");
	Symbol containsDigitAndSlash(L":containsDigitAndSlash");
	Symbol containsDigitAndComma(L":containsDigitAndComma");
	Symbol containsDigitAndPeriod(L":containsDigitAndPeriod");
	Symbol containsDigitAndPunct(L":containsDigitAndPunct");
	Symbol containsAlphaAndPunct(L":containsAlphaAndPunct");
	Symbol containsAtSign(L":containsAtSign");
	Symbol lowerCase(L":lowercase");
	Symbol punctuation(L":punctuation");
	/*
	Symbol containsDigitAndAlpha;
	Symbol containsDigitAndDash;
	Symbol containsDigitAndSlash;
	Symbol containsDigitAndComma;
	Symbol containsDigitAndPeriod;
	Symbol containsDigitAndPunct;
	Symbol containsAlphaAndPunct;
	Symbol containsAtSign;
	Symbol containsAscii;
	Symbol number;
	Symbol otherNum;
	Symbol allCaps;
	Symbol capPeriod;
	Symbol firstWordInVocabAsLowercase;
	Symbol firstWord;
	Symbol initCap;
	Symbol lowerCase;
	Symbol otherFeatures;
	Symbol punctuation;
	Symbol bracket;
	*/

}

DefaultIdFWordFeatures::DefaultIdFWordFeatures(int mode, IdFListSet *listSet)
{
	_mode = mode;
	_listSet = listSet;
}

int DefaultIdFWordFeatures::getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const
{
	if (_mode == TIMEX){
		return getAllTimexFeatures(wordSym, first_word, normalized_word_is_in_vocab,
			featureArray, max_features);
	}
	else if (_mode == OTHER_VALUE) {
		return getAllOtherValueFeatures(wordSym, first_word, normalized_word_is_in_vocab,
			featureArray, max_features);
	}
	
	int num_features = 0;

	// list feature
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null())
			featureArray[num_features++] = listFeature;
	}

	if (num_features >= max_features) {
		return num_features;}

	// We'll want to account for digits and punctuation any
	// time we're using a roman script
	// Digits and punctuation may or may not occur in
	// non-roman scripts, but if they do, they are
	// probably good features to use
	std::wstring word = wordSym.to_string();
	std::wstring::size_type length = word.length();

	bool contains_digit = false;
	bool contains_non_digit = false;
	bool contains_punct = false;
	bool contains_non_punct = false;
	bool contains_dash = false;
	bool contains_comma = false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	bool contains_at = false;
	for (unsigned i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			contains_digit = true;
		else contains_non_digit = true;

		if (iswpunct(word[i]))
			contains_punct = true;
		else contains_non_punct = true;

		if (word[i] == L'.')
			contains_period = true;
		else if (word[i] == L'-')
			contains_dash = true;
		else if (word[i] == L',')
			contains_comma = true;
		else if (word[i] == L'/')
			contains_slash = true;
		else if (word[i] == L'@')
			contains_at = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;
	}
	// adjust the features for special tokens, like "-LRB-"
	if (SymbolUtilities::isBracket(wordSym)) {
		contains_alpha = false;
		contains_dash = false;
		contains_non_punct = false;
		featureArray[num_features++] = bracket;
		if (num_features >= max_features) return num_features;
	}

	if (!contains_non_punct) {
		featureArray[num_features++] = punctuation;
		if (num_features >= max_features) 
			return num_features;
	}	

	// Simplify the number feature (compared to en version):
	// if all digits, it's a number
	if (contains_digit){
		if(!contains_non_digit)
			featureArray[num_features++] = number;

		// Combination digit+ features
		else {
			if (contains_alpha) {
				featureArray[num_features++] = containsDigitAndAlpha;
				if (num_features >= max_features) return num_features;
			}
			if (contains_dash) {
				featureArray[num_features++] = containsDigitAndDash;
				if (num_features >= max_features) return num_features;
			}
			if (contains_slash) {
				featureArray[num_features++] = containsDigitAndSlash;
				if (num_features >= max_features) return num_features;
			}
			if (contains_comma) {
				featureArray[num_features++] = containsDigitAndComma;
				if (num_features >= max_features) return num_features;
			}
			if (contains_period) {
				featureArray[num_features++] = containsDigitAndPeriod;
				if (num_features >= max_features) return num_features;
			}
			if (contains_punct) {
				featureArray[num_features++] = containsDigitAndPunct;
				if (num_features >= max_features) return num_features;
			}
		}
	}
	// Combination punctuation+ features
	if (contains_at) {
		featureArray[num_features++] = containsAtSign;
		if (num_features >= max_features) return num_features;
	}
	
	if (contains_alpha && contains_punct) {
		featureArray[num_features++] = containsAlphaAndPunct;
		if (num_features >= max_features) return num_features;
	}

	if (contains_alpha && !contains_digit) {
		Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
		if (!caseFeature.is_null()) {
			featureArray[num_features++] = caseFeature;
			if (num_features >= max_features) return num_features;
		}
	}

	//checking for ascii might be useful for non-roman scripts
		
	if (containsASCII(wordSym)){
		featureArray[num_features++] = containsAscii;
		if (num_features >= max_features) return num_features;
	}

	if (num_features==0)
		featureArray[num_features++] = otherFeatures;
	return num_features;
}

Symbol DefaultIdFWordFeatures::features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	if (_mode == TIMEX){
		return getTimexFeature(wordSym, first_word, normalized_word_is_in_vocab);
	}
	else if (_mode == OTHER_VALUE) {
		return getOtherValueFeature(wordSym, first_word, normalized_word_is_in_vocab);
	}
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null()){
			return listFeature;
		}
	}
	std::wstring word = wordSym.to_string();
	std::wstring::size_type length = word.length();

	// simplified number feature
	bool contains_digit = false;
	bool contains_non_digit = false;
	for (unsigned i=0; i< length; ++i){
		if (iswdigit(word[i]))
			contains_digit = true;
		else
			contains_non_digit = true;
	}
	if (contains_digit && !contains_non_digit)
		return number;

	contains_digit = false;
	bool contains_dash = false;
	bool contains_comma= false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	for (unsigned i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			contains_digit = true;
		else if (word[i] == L'.')
			contains_period = true;
		else if (word[i] == L'-')
			contains_dash = true;
		else if (word[i] == L',')
			contains_comma = true;
		else if (word[i] == L'/')
			contains_slash = true;
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
		else if (contains_period)
			return containsDigitAndPeriod;
		else return otherNum;
	}

	if (containsASCII(wordSym))
		return containsAscii;

	Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
	if (!caseFeature.is_null())
		return caseFeature;

	return otherFeatures;
		
}

bool DefaultIdFWordFeatures::containsDigit(Symbol wordSym) const
{
	std::wstring word = wordSym.to_string();

	std::wstring::size_type length = word.length();

	for (size_t i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			return true;
	}

	return false;
	
}

Symbol DefaultIdFWordFeatures::normalizedWord(Symbol wordSym) const
{
	std::wstring str = wordSym.to_string();
	std::wstring::size_type length = str.length();
	for (unsigned i = 0; i < length; ++i) {
		str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
	
}

Symbol DefaultIdFWordFeatures::extendedFeatures(Symbol wordSym, bool first_word) const 
{
	std::wstring str = wordSym.to_string();
	std::wstring::size_type length = str.length();
	std::wstring copy = std::wstring(str);
	for (unsigned i = 0; i < length; i++) {
		if(iswlower(copy[i])) {
			copy[i] = L'x';
			continue;
		}else if(iswupper(copy[i])) {
            copy[i] = L'X';
			continue;
		}else if(iswdigit(copy[i])) {
			copy[i] = L'9';
			continue;
		}/*else if(iswpunct(copy[i])) {
			copy[i] = L'-';
			continue;
		}*/
	}
	return Symbol(copy.c_str());
}

Symbol DefaultIdFWordFeatures::getCaseFeature(std::wstring word, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	std::wstring::size_type length = word.length();

	// whether we are looking at the first word is language-independent
	if (first_word)
		return firstWord;

	// other case rules only apply
	// to Roman-script languages, but there are a few of those
	bool wordIsAllUpperCase = true;
    for (unsigned j = 0; j < length; ++j) {
        if (iswlower(word[j])) 
            wordIsAllUpperCase = false;
    }

	if (wordIsAllUpperCase)
		return allCaps;

    if (iswupper(word[0])) {
		if (length == 2 && word[1] == L'.')
			return capPeriod;
		else if (normalized_word_is_in_vocab && first_word)
			return firstWordInVocabAsLowercase;
		else return initCap;
    }
	
	if (iswlower(word[0]))
		return lowerCase;
		
	return Symbol();

}

Symbol DefaultIdFWordFeatures::getTimexFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	std::wstring word = wordSym.to_string();
	std::wstring::size_type length = word.length();
	
	bool contains_digit = false;
	bool contains_non_digit = false;
	bool contains_punct = false;
	bool contains_non_punct = false;
	bool contains_dash = false;
	bool contains_comma= false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	for (unsigned i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			contains_digit = true;
		else contains_non_digit = true;

		if (iswpunct(word[i]))
			contains_punct = true;
		else contains_non_punct = true;

		if (word[i] == L'.')
			contains_period = true;
		else if (word[i] == L'-')
			contains_dash = true;
		else if (word[i] == L',')
			contains_comma = true;
		else if (word[i] == L'/')
			contains_slash = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;
	}

	if (!contains_non_punct)
		return punctuation;


	if (contains_digit) {
		if (contains_alpha)
			return containsDigitAndAlpha;
		else if (contains_dash)
			return containsDigitAndDash;
		else if (contains_slash)
			return containsDigitAndSlash;
		else if (contains_comma)
			return containsDigitAndComma;
		else if (contains_period)
			return containsDigitAndPeriod;
		else 
			if (!contains_non_digit) 
				return number;
			
			
		return otherNum;
	}
		

	Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
	if (!caseFeature.is_null())
		return caseFeature;

	return otherFeatures;	
}

Symbol DefaultIdFWordFeatures::getOtherValueFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	std::wstring word = wordSym.to_string();
	std::wstring::size_type length = word.length();
	
	bool contains_digit = false;
	bool contains_non_digit = false;
	bool contains_punct = false;
	bool contains_non_punct = false;
	bool contains_dash = false;
	bool contains_comma= false;
	bool contains_alpha = false;
	bool contains_slash = false;
	bool contains_period = false;
	for (unsigned i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			contains_digit = true;
		else contains_non_digit = true;

		if (iswpunct(word[i]))
			contains_punct = true;
		else contains_non_punct = true;

		if (word[i] == L'.')
			contains_period = true;
		else if (word[i] == L'-')
			contains_dash = true;
		else if (word[i] == L',')
			contains_comma = true;
		else if (word[i] == L'/')
			contains_slash = true;
		else if (iswlower(word[i]) || iswupper(word[i]))
			contains_alpha = true;
	}

	if (!contains_non_punct)
		return punctuation;

	if (contains_digit) {
		if (contains_alpha)
			return containsDigitAndAlpha;
		else if (contains_dash)
			return containsDigitAndDash;
		else if (contains_slash)
			return containsDigitAndSlash;
		else if (contains_comma)
			return containsDigitAndComma;
		else if (contains_period)
			return containsDigitAndPeriod;
		else return otherNum;
	}	

	Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
	if (!caseFeature.is_null())
		return caseFeature;

	return otherFeatures;
}

bool DefaultIdFWordFeatures::containsASCII(Symbol wordSym) const
{
	std::wstring word = wordSym.to_string();
	size_t length = word.length();
	for(unsigned i = 0; i<  length; ++i){
		if(iswascii(word[i])){
			return true;
		}
	}
	return false;
}
