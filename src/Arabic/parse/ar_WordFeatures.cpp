// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/parse/ar_WordFeatures.h"
#ifdef _WIN32
	#define swprintf _snwprintf
#endif
const int ArabicWordFeatures::_numPrefix = WORD_FEATURES_NUM_PREF;
const int ArabicWordFeatures::_numSuffix = WORD_FEATURES_NUM_SUFF;
/*
* Arabic Word Features depend on the morphological analysis.  The current Word Features
* assume words are unanalyzed, except the removal of clitic prepostions, pronouns, conjuntions
* and particles.  Consequently words do not have vowels, and the inflectional morphology is
* a part of the word.  
*
*/
ArabicWordFeatures::ArabicWordFeatures()
{
	_suffix[0] = L"\x062a\x064a\x0646"; // L"tyn";
	_suffix[1] = L"\x062a\x064a"; // L"ty";
	_suffix[2] = L"\x064a\x0646"; // L"yn";
	_suffix[3] = L"\x0648\x0646"; // L"wn";
	_suffix[4] = L"\x0627\x0646"; // L"An";
	_suffix[5] = L"\x062a\x0627"; // L"tA";
	_suffix[6] = L"\x0622\x062a"; // L"|t";
	_suffix[7] = L"\x0627\x062a"; // L"At";
	_suffix[8] = L"\x0627"; // L"A";
	_suffix[9] = L"\x0629"; // L"p";
	_suffix[10]= L"\x062a"; // L"t";
	//These are clitics, usally they are dettached from the word!
	/*
	_suffix[11] = ArabicSymbol(L"hmA");
	_suffix[12] = ArabicSymbol(L"nHn");
	_suffix[13] = ArabicSymbol(L"hA");
	_suffix[14] = ArabicSymbol(L"hn");
	_suffix[15] = ArabicSymbol(L"km");
	_suffix[16] = ArabicSymbol(L"nA");
	_suffix[17] = ArabicSymbol(L"ny");
	_suffix[18] = ArabicSymbol(L"mA");
	_suffix[19] = ArabicSymbol(L"h");
	_suffix[20] = ArabicSymbol(L"k");
	_suffix[21] = ArabicSymbol(L"y");
	*/
	_prefix[0] = L"\x0623"; // L">";
	_prefix[1] = L"\x0627\x0644"; // L"Al";
	_prefix[2] = L"\x0646"; // L"n";
	_prefix[3] = L"\x0633"; // L"s";
	_prefix[4] = L"\x062a"; // L"t";
	_prefix[5] = L"\x064a"; // L"y";
	_prefix[6] = L"\x0622"; // L"|";
	//These are clitics, usally dettached from the word
	/*
	_prefix[7] = ArabicSymbol(L"b");
	_prefix[8] = ArabicSymbol(L"f");
	_prefix[9] = ArabicSymbol(L"k");
	_prefix[10] = ArabicSymbol(L"l");
	_prefix[11] = ArabicSymbol(L"w");
	*/
}
std::wstring ArabicWordFeatures::prefixFeature(const std::wstring& word) const{
	//Assume this is dashless text
	for (unsigned i =0; i< (size_t)_numPrefix; i++){
		std::wstring feature(_prefix[i]);
		size_t j;
		if(word.length()>feature.length()){
			if(((j = word.find(feature)) != string::npos) && (j == 0)){
				wchar_t cstr[4];
				swprintf(cstr, 4, L"P%u", i+1);
				return wstring(cstr);
			}
		}
	}
	return L"P0";
}
std::wstring ArabicWordFeatures::suffixFeature(const std::wstring& word) const{
	//Assume there are no dashes
	for (unsigned i = 0; i < (size_t)_numSuffix; i++) {
		std::wstring feature(_suffix[i]);
		if(word.length() > feature.length()){
            size_t j;
            if (((j = word.rfind(feature)) != string::npos) &&
	        (j == (word.length() - feature.length()))) {
				wchar_t cstr[4];
 				swprintf(cstr, 4, L"S%u", i+1);
				return std::wstring(cstr);
            }
		}
	}
	return L"S0";
}

bool ArabicWordFeatures::isNumber(const std::wstring& word) const
{
	unsigned i = 0;
	size_t length = word.length();
	bool found_number = false;
	
	if ( (word[0] == L'+') || (word[0] == L'-') )
		i++;
	for ( ; i < length; ++i) {
		if (isascii(word[i])) {
			if (isdigit(word[i])) {
				found_number = true;
				continue;
			}
			if (word[i] == L'.')
				continue;
			if ( (tolower(word[i]) == L'e') &&
				(i > 0) && 
				isdigit(word[i-1]) &&
				(i < (length - 1)) )
				continue;
			if ( (tolower(word[i]) == L'd') &&
				(i == (length - 1)) )
				continue;
			return false;
		}
		else {
			if ((word[i] >= L'\x0660' && word[i] <= L'\x0669') ||
				(word[i] >= L'\x06f0' && word[i] <= L'\x06f9') ||
				(word[i] >= L'\x0966' && word[i] <= L'\x096f'))
			{
				found_number = true;
			}
			else {
				return false;
			}
		}
	}
	if (found_number) {
		return true;
	} else return false;
}

//TODO:!!!! This feature set is not the same as the Arabic_Parser feature set- 
//1) Arabic ArabicParser always uses hyphenization (H0)
//2) Arabic number Features still have R0 in them
//RETRAIN BEFORE Running!
Symbol ArabicWordFeatures::features(Symbol wordSym, bool fw = false) const
{
	std::wstring wordStr = wordSym.to_string();
	std::wstring returnStr;
    if (isNumber(wordStr)) {
        returnStr = L"P0S0N1";
    } 
	else {
       	returnStr += prefixFeature(wordStr);
		returnStr += suffixFeature(wordStr);
        returnStr += L"N0";
    }
    return Symbol(returnStr.c_str());
}
//Same changes necessary here,
Symbol ArabicWordFeatures::reducedFeatures(Symbol wordSym, bool fw = false) const
{
	std::wstring wordStr = wordSym.to_string();
	std::wstring returnStr;
    if (isNumber(wordStr)) {
        returnStr = L"P0S0N1";
    } else {
        returnStr = L"P0S0N0";
      }
    return Symbol(returnStr.c_str());
}
