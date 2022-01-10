// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/names/en_IdFWordFeatures.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/common/SymbolUtilities.h"
#include <wchar.h>
#include <iostream>
#include "Generic/linuxPort/serif_port.h"

using namespace std;

Symbol EnglishIdFWordFeatures::twoDigitNum = Symbol(L":twoDigitNum");
Symbol EnglishIdFWordFeatures::fourDigitNum = Symbol(L":fourDigitNum");
Symbol EnglishIdFWordFeatures::containsDigitAndAlpha = Symbol(L":containsDigitAndAlpha");
Symbol EnglishIdFWordFeatures::containsDigitAndDash = Symbol(L":containsDigitAndDash");
Symbol EnglishIdFWordFeatures::containsDigitAndSlash = Symbol(L":containsDigitAndSlash");
Symbol EnglishIdFWordFeatures::containsDigitAndComma = Symbol(L":containsDigitAndComma");
Symbol EnglishIdFWordFeatures::containsDigitAndPeriod = Symbol(L":containsDigitAndPeriod");
Symbol EnglishIdFWordFeatures::containsDigitAndPunct = Symbol(L":containsDigitAndPunct");
Symbol EnglishIdFWordFeatures::containsAlphaAndPunct = Symbol(L":containsAlphaAndPunct");
Symbol EnglishIdFWordFeatures::containsAtSign = Symbol(L":containsAtSign");
Symbol EnglishIdFWordFeatures::otherNum = Symbol(L":otherNum");
Symbol EnglishIdFWordFeatures::allCaps = Symbol(L":allCaps");
Symbol EnglishIdFWordFeatures::capPeriod = Symbol(L":capPeriod");
Symbol EnglishIdFWordFeatures::firstWord = Symbol(L":firstWord");
Symbol EnglishIdFWordFeatures::firstWordInVocabAsLowercase = Symbol(L":firstWordInVocabAsLowercase");
Symbol EnglishIdFWordFeatures::initCap = Symbol(L":initCap");
Symbol EnglishIdFWordFeatures::lowerCase = Symbol(L":lowerCase");
Symbol EnglishIdFWordFeatures::otherFeatures = Symbol(L":otherFeatures");
Symbol EnglishIdFWordFeatures::punctuation = Symbol(L":punctuation");
Symbol EnglishIdFWordFeatures::bracket = Symbol(L":bracket");
Symbol EnglishIdFWordFeatures::formulaicTime = Symbol(L":formulaicTime");

// more features (from ne_intfeatures.h/NE_ExpFeature)
// tried, but not currently used because they didn't help,
// especially since we're not finding dates/times in SERIF.
Symbol EnglishIdFWordFeatures::oneToTwelve = Symbol(L":oneToTwelve");
Symbol EnglishIdFWordFeatures::oneToThirtyOne = Symbol(L":oneToThirtyOne");
Symbol EnglishIdFWordFeatures::zeroToSixty = Symbol(L":zeroToSixty");
Symbol EnglishIdFWordFeatures::otherTwoDigitNum = Symbol(L":otherTwoDigitNum");
Symbol EnglishIdFWordFeatures::yearNum = Symbol(L":yearNum");

EnglishIdFWordFeatures::EnglishIdFWordFeatures(int mode, IdFListSet *listSet) {
	_mode = mode;
	_listSet = listSet;
}

bool EnglishIdFWordFeatures::containsDigit(Symbol wordSym) const
{
	std::wstring word = wordSym.to_string();
	
	wstring::size_type length = word.length();

	for (size_t i = 0; i < length; ++i) {
		if (iswdigit(word[i]))
			return true;
	}

	return false;
}

int EnglishIdFWordFeatures::getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const 
{
	if (_mode == TIMEX)
		return getAllTimexFeatures(wordSym, first_word, normalized_word_is_in_vocab,
			featureArray, max_features);
	else if (_mode == OTHER_VALUE)
		return getAllOtherValueFeatures(wordSym, first_word, normalized_word_is_in_vocab,
			featureArray, max_features);

	// possible features:
	// 1) list feature
	// 2) bracket
	// 3) punctuation
	// 4) digit feature: ANY OF (twoDigitNum 
	//                           fourDigitNum
	//                           oneToTwelve
	//                           oneToThirtyOne
	//                           zeroToSixty
	//                           yearNum
	//                           otherNum) OR
	//                   ANY OF (containsDigitAndAlpha
	//                           containsDigitAndComma
	//                           containsDigitAndDash
	//                           containsDigitAndPeriod
	//                           containsDigitAndSlash
	//                           containsDigitAndPunct)
	// 5) contains @
	// 6) contains alpha and punct
	// 7) case feature: allCaps OR capPeriod OR initCap OR 
	//					firstWord OR firstWordInVocabAsLowercase OR lowercase
	// 8) extended feature

	int num_features = 0;

	// list feature
	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null())
			featureArray[num_features++] = listFeature;
	}

	if (num_features >= max_features) return num_features;

	std::wstring word = wordSym.to_string();
	wstring::size_type length = word.length();

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
		if (num_features >= max_features) return num_features;
	}
	
	// get number feature
	if (contains_digit){
		if (!contains_non_digit) {
			int n_digit_features = 0;
			int word_as_number = _wtoi(word.c_str());
			if (length == 2) {
				featureArray[num_features++] = twoDigitNum;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}
			if (length == 4) {
				featureArray[num_features++] = fourDigitNum;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}
			if ((word_as_number >= 1) && (word_as_number <= 12)) {
				featureArray[num_features++] = oneToTwelve;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}
			if ((word_as_number >= 1) && (word_as_number <= 31)) {
				featureArray[num_features++] = oneToThirtyOne;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}
			if ((word_as_number >= 0) && (word_as_number <= 60)) {
				featureArray[num_features++] = zeroToSixty;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}
			if ((1900 <= word_as_number) && (word_as_number <= 2019)) { // Note another place where years are defined is en_WordConstants::isFourDigitYear()
				featureArray[num_features++] = yearNum;
				if (num_features >= max_features) return num_features;
				n_digit_features++;
			}

			if (n_digit_features == 0) {
				featureArray[num_features++] = otherNum;
				if (num_features >= max_features) return num_features;
			}
		} else {
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

	if ((contains_alpha && contains_digit) ||
		(contains_alpha && contains_punct) ||
		(contains_digit && contains_non_digit) ||
		(contains_punct && contains_non_punct))
	{
		featureArray[num_features++] = extendedFeatures(wordSym, first_word);
		if (num_features >= max_features) return num_features;
	}
		
	//std::cerr << wordSym.to_debug_string() << ": ";
	//for (int i = 0; i < num_features; i++) {
	//	std::cerr << featureArray[i].to_debug_string() << " ";
	//}
	//std::cerr << "\n";

	return num_features;
}

Symbol EnglishIdFWordFeatures::getCaseFeature(std::wstring word, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	wstring::size_type length = word.length();

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
		else if (first_word)
			return firstWord;
		else return initCap;
    }
	
	if (iswlower(word[0]))
		return lowerCase;
		
	return Symbol();

}


Symbol EnglishIdFWordFeatures::features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	if (_mode == TIMEX)
		return getTimexFeature(wordSym, first_word, normalized_word_is_in_vocab);
	else if (_mode == OTHER_VALUE)
		return getOtherValueFeature(wordSym, first_word, normalized_word_is_in_vocab);

	if (_listSet != 0) {
		Symbol listFeature = _listSet->getFeature(wordSym);
		if (!listFeature.is_null())
			return listFeature;
	}

	std::wstring word = wordSym.to_string();
	
	wstring::size_type length = word.length();
	
	// original IdF features
	if (length == 2 && iswdigit(word[0]) && iswdigit(word[1]))
		return twoDigitNum;
	if (length == 4 && iswdigit(word[0]) && iswdigit(word[1]) &&
		iswdigit(word[2]) && iswdigit(word[3]))
		return fourDigitNum;

	bool contains_digit = false;
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

	Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
	if (!caseFeature.is_null())
		return caseFeature;

	return otherFeatures;
	
	
}

Symbol EnglishIdFWordFeatures::getTimexFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	std::wstring word = wordSym.to_string();
	wstring::size_type length = word.length();
	
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

	/*if (length == 8 && !contains_non_digit &&
		(word[4] == '0' || word[4] == '1'))
		return formulaicTime;

	size_t t_index = word.find(L"T");
	if (t_index >= 0 && t_index + 8 < length &&
		iswdigit(word[t_index+1]) && iswdigit(word[t_index+2]) &&
		word[t_index+3] == L':' &&
		iswdigit(word[t_index+4]) && iswdigit(word[t_index+5]) &&
		word[t_index+6] == L':' &&
		iswdigit(word[t_index+7]) && iswdigit(word[t_index+8]))
		return formulaicTime;*/

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
		else {
			if (!contains_non_digit) {
				if (length == 1 && word[0] == L'0')
					return zeroToSixty;
				int word_as_number = _wtoi(word.c_str());
				if (word_as_number >= 1) {
					if (word_as_number <= 12)
						return oneToTwelve;
					else if (word_as_number <= 31)
						return oneToThirtyOne;
					else if (word_as_number <= 60)
						return zeroToSixty;
					else if (length == 2)
						return otherTwoDigitNum;
					else if ((1900 <= word_as_number) && (word_as_number <= 2019)) // Note another place where years are defined is en_WordConstants::isFourDigitYear()
						return yearNum;
					else return otherNum;
				}
			}
			return otherNum;
		}
	}	

	Symbol caseFeature = getCaseFeature(word, first_word, normalized_word_is_in_vocab);
	if (!caseFeature.is_null())
		return caseFeature;

	return otherFeatures;	
}

Symbol EnglishIdFWordFeatures::getOtherValueFeature(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const
{
	std::wstring word = wordSym.to_string();
	wstring::size_type length = word.length();
	
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

/** returns lowercase version of wordSym */
Symbol EnglishIdFWordFeatures::normalizedWord(Symbol wordSym) const {
	std::wstring str = wordSym.to_string();
	wstring::size_type length = str.length();
    for (unsigned i = 0; i < length; ++i) {
        str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());

}

Symbol EnglishIdFWordFeatures::extendedFeatures(Symbol wordSym, bool first_word) {
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
