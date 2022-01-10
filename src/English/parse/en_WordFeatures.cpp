// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_WordFeatures.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


using namespace std;


const unsigned EnglishWordFeatures::numDerivationalFeatures = 
    WORD_FEATURES_NUM_DERIV_FEATURES;

EnglishWordFeatures::EnglishWordFeatures()
{
    derivationalFeatures[0] = L"-backed";
    derivationalFeatures[1] = L"-based";
    derivationalFeatures[2] = L"graphy";
    derivationalFeatures[3] = L"meter";
    derivationalFeatures[4] = L"ente";
    derivationalFeatures[5] = L"ment";
    derivationalFeatures[6] = L"ness";
    derivationalFeatures[7] = L"tion";
    derivationalFeatures[8] = L"ael";
    derivationalFeatures[9] = L"ary";
    derivationalFeatures[10] = L"ate";
    derivationalFeatures[11] = L"ble";
    derivationalFeatures[12] = L"ent";
    derivationalFeatures[13] = L"ess";
    derivationalFeatures[14] = L"est";
    derivationalFeatures[15] = L"ial";
    derivationalFeatures[16] = L"ian";
    derivationalFeatures[17] = L"ine";
    derivationalFeatures[18] = L"ion";
    derivationalFeatures[19] = L"ism";
    derivationalFeatures[20] = L"ist";
    derivationalFeatures[21] = L"ite";
    derivationalFeatures[22] = L"ity";
    derivationalFeatures[23] = L"ive";
    derivationalFeatures[24] = L"ize";
    derivationalFeatures[25] = L"nce";
    derivationalFeatures[26] = L"ogy";
    derivationalFeatures[27] = L"ous";
    derivationalFeatures[28] = L"sis";
    derivationalFeatures[29] = L"uan";
    derivationalFeatures[30] = L"al";
    derivationalFeatures[31] = L"an";
    derivationalFeatures[32] = L"as";
    derivationalFeatures[33] = L"er";
    derivationalFeatures[34] = L"ez";
    derivationalFeatures[35] = L"ia";
    derivationalFeatures[36] = L"ic";
    derivationalFeatures[37] = L"ly";
    derivationalFeatures[38] = L"on";
    derivationalFeatures[39] = L"or";
    derivationalFeatures[40] = L"os";
    derivationalFeatures[41] = L"um";
    derivationalFeatures[42] = L"us";
    derivationalFeatures[43] = L"a";
    derivationalFeatures[44] = L"i";
    derivationalFeatures[45] = L"o";
    derivationalFeatures[46] = L"y";

    for (int i = 0; i < 256; i++) {
        consonants[i] = false;
    }
    consonants[L'b'] = true;
    consonants[L'c'] = true;
    consonants[L'd'] = true;
    consonants[L'f'] = true;
    consonants[L'g'] = true;
    consonants[L'h'] = true;
    consonants[L'j'] = true;
    consonants[L'k'] = true;
    consonants[L'l'] = true;
    consonants[L'm'] = true;
    consonants[L'n'] = true;
    consonants[L'p'] = true;
    consonants[L'q'] = true;
    consonants[L'r'] = true;
    consonants[L's'] = true;
    consonants[L't'] = true;
    consonants[L'v'] = true;
    consonants[L'w'] = true;
    consonants[L'x'] = true;
    consonants[L'y'] = true;
    consonants[L'z'] = true;
}

bool EnglishWordFeatures::isConsonant(wchar_t ch) const
{
    return consonants[ch];
}


bool EnglishWordFeatures::sInflection(const wstring& word) const
{
	wstring::size_type length = word.length();

    if (length > 2) {
        wchar_t lastChar = word[length - 1];
        wchar_t nextToLast = word[length - 2];
        return ( (lastChar == L's')   && (nextToLast != L's') &&
            ((nextToLast == L'e') || isConsonant(nextToLast)) );
    } else {
        return false;
    }
}


wstring EnglishWordFeatures::capitalizationFeature(const wstring& word, bool firstWord)
    const
{
    bool wordIsAllUpperCase = true;
	wstring::size_type length = word.length();
    for (size_t i = 0; i < length; ++i) {
        if (iswlower(word[i])) {
            wordIsAllUpperCase = false;
            break;
        }
    }
    if (iswupper(word[0])) {
        if (firstWord) {
            return L"C1";
        } else if (wordIsAllUpperCase) {
            for (unsigned i = 0; i < length; ++i) {
	        if (iswdigit(word[i])) {
	            return L"C2";
	        }
            }
            return L"C3";
        } else {
            return L"C4";
        }
    }
    return L"C0";
}

wstring EnglishWordFeatures::hyphenizationFeature(const wstring& word) const
{
	wstring::size_type length = word.length();
    for (unsigned i = 0; i < length; ++i) {
        if (word[i] == L'-') {
            return L"H1";
        }
    }
    return L"H0";
}


wstring EnglishWordFeatures::inflectionalFeature(const wstring& word) const
{
	wstring::size_type i;
	wstring::size_type length = word.length();
    if (length > 3) {
        if (sInflection(word)) {
            return L"I1";
        } else if (((i = word.rfind(L"ed")) != wstring::npos) &&
            (i == (length - 2))) {
            return L"I2";
        } else if (((i = word.rfind(L"ing")) != wstring::npos) &&
            (i == (length - 3))) {
            return L"I3";
        }
    }
    return L"I0";
}


wstring EnglishWordFeatures::derivationalFeature(const wstring& word) const
{
	wstring::size_type length = word.length();
    if (length > 6) {
        wstring wordTrunc;
        if (sInflection(word)) {
            wordTrunc = word.substr(0, length - 1);
        } else {
            wordTrunc = word;
        }
        for (unsigned i = 0; i < numDerivationalFeatures; ++i) {
			wstring::size_type j;
            wstring feature(derivationalFeatures[i]);
            if (((j = wordTrunc.rfind(feature)) != wstring::npos) &&
	        (j == (wordTrunc.length() - feature.length()))) {
	        wchar_t cstr[5];
 	        swprintf(cstr, 5, L"D%u", i+1);
 	        return wstring(cstr);
            }
        }
    }
    return L"D0";
}

bool EnglishWordFeatures::isNumber(const wstring& word) const
{
	unsigned i = 0;
	wstring::size_type length = word.length();
	bool found_number = false;
	
	if ( (word[0] == L'+') || (word[0] == L'-') || (word[0] == L'#') )
		i++;
	for ( ; i < length; ++i) {
		if (iswdigit(word[i])) {
			found_number = true;
			continue;
		}
		if (word[i] == L'.')
			continue;
		// LB: allow ',' -- as in 1,000
		if (word[i] == L',')
			continue;
		// LB: allow m to be the last character -- as in 200m
		if (towlower(word[i]) == L'm' && i == length - 1)
			continue;
		// LB: allow bn to be the last two characters -- as in 200bn
		if (towlower(word[i]) == L'b' && 
			i == length - 2 &&
			towlower(word[i+1]) == L'n') 
		{
			i++;
			continue;
		}		
		if ( (towlower(word[i]) == L'e') &&
			(i > 0) && 
			iswdigit(word[i-1]) &&
			(i < (length - 1)) )
			continue;
		if ( (towlower(word[i]) == L'd') &&
			(i == (length - 1)) )
			continue;
		return false;
	}
	if (found_number) {
		return true;
	} else return false;
}


Symbol EnglishWordFeatures::features(Symbol wordSym, bool firstWord) const
{
    wstring wordStr = wordSym.to_string();
    wstring returnStr;
    if (isNumber(wordStr)) {
        returnStr = L"C0H0I0D0N1";
    } else {
        returnStr += capitalizationFeature(wordStr, firstWord);
        returnStr += hyphenizationFeature(wordStr);
        returnStr += inflectionalFeature(wordStr);
        returnStr += derivationalFeature(wordStr);
        returnStr += L"N0";
      }
    return Symbol(returnStr.c_str());
}

Symbol EnglishWordFeatures::reducedFeatures(Symbol wordSym, bool firstWord) const
{
    wstring wordStr = wordSym.to_string();
    wstring returnStr;
    if (isNumber(wordStr)) {
        returnStr = L"C0H0I0D0N1";
    } else {
        returnStr += capitalizationFeature(wordStr, firstWord);
        returnStr += L"H0I0D0N0";
      }
    return Symbol(returnStr.c_str());
}

