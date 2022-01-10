// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/edt/ar_BaseFormMaker.h"
#include <string>
#include <cstdio>
#include <stdlib.h>
#include <cstddef>
#include <iostream>



using namespace std;

Symbol BaseFormMaker::_Al = ArabicSymbol(L"Al");
Symbol BaseFormMaker::_l = ArabicSymbol(L"l");
Symbol BaseFormMaker::_w = ArabicSymbol(L"w");
Symbol BaseFormMaker::_b = ArabicSymbol(L"b");



Symbol BaseFormMaker::_stAn = ArabicSymbol(L"stAn");
wchar_t BaseFormMaker::_wordBuffer[MAX_WORD_BUFFER_SZ];
Symbol BaseFormMaker::_endings[] = 
{
	ArabicSymbol(L"ytAn"),
	ArabicSymbol(L"ytyn"),
	ArabicSymbol(L"yyn"),
	ArabicSymbol(L"yAn"),
	ArabicSymbol(L"ywn"),
	ArabicSymbol(L"yAt"),
	ArabicSymbol(L"yA"),
	ArabicSymbol(L"yw"),
	ArabicSymbol(L"yy"),
	ArabicSymbol(L"yp"),
	ArabicSymbol(L"y"),
	ArabicSymbol(L"A"),
	ArabicSymbol(L"stAn")
};

Symbol BaseFormMaker::removeNameClitics(Symbol word){
	const wchar_t*  wordStr = word.to_string();
	size_t strLen = wcslen(wordStr);
	if(strLen < 3){
		return word;
	}
	if(strLen >= (MAX_WORD_BUFFER_SZ-1)){ //weird word its too long for a substring...
		return word;
	}
	int startIndex =0;
	if(startsWith(wordStr,_w,0) || 
		startsWith(wordStr,_l,0) ||
		startsWith(wordStr, _b, 0))
	{
		startIndex++;
		if(startsWith(wordStr,_w,1) || 
			startsWith(wordStr,_l,1) ||
			startsWith(wordStr, _b, 1))
			{
				startIndex++;
			}
		unsigned i = 0;
    	for(i=startIndex; i < strLen; i++){
			_wordBuffer[i-startIndex] = wordStr[i];
		}
		_wordBuffer[i-startIndex]=L'\0';
		return Symbol(_wordBuffer);
	}
	else{
		return word;
	}


}
Symbol BaseFormMaker::getPERBaseForm(Symbol word){
	return getGPEBaseForm(word);
}
Symbol BaseFormMaker::getGPEBaseForm(Symbol word){
	const wchar_t* wordStr = word.to_string();
	size_t startIndex = 0;
	size_t endIndex = wcslen(wordStr);
	size_t strLen = wcslen(wordStr);
	size_t i;
	
	if(strLen < 3)
		return word;
	if(strLen >= (MAX_WORD_BUFFER_SZ-1)){ //weird word its too long for a substring...
		return word;
	}
	//Note: _Al, _l removals are done twice
	//this hack is for cases like "Germany" AlAlmAny and AlmAny
	//AlAlmAny is The Germans, AlmAny is Germans
	//the correct root is AlmAn, but since Al is removed as 'the'
	//AlmAny becomes mAn.  the hack creates an artificial (incorrect) 
	//root, mAn, in all cases
	//the same principle applies for 'l' and Lebannon 
	if(strLen >=5){
		if(startsWith(wordStr, _Al, startIndex)){
			startIndex = 2;
			if(strLen - startIndex >=5){
				if(startsWith(wordStr, _Al, startIndex)){
					startIndex+=2;
				}
			}
		}
	}	
	/*
	if(strLen - startIndex >= 4){
		if(startsWith(wordStr, _l, startIndex)){
			startIndex+=1;
			if(strLen - startIndex >= 4){
				if(startsWith(wordStr, _l, startIndex)){
					startIndex+=1;
				}
			}
		}
	}
	*/
	//endings only occur once, and per inspection don't end names, 
	//no need for double search
	for(i =0; i< (size_t)_numEndings; i++){
		if(endsWith(wordStr, _endings[i], endIndex)){
			size_t len = wcslen(_endings[i].to_string());
			endIndex-=(len);
			break;
		}
	}
	//look for stAn- this isn't an inflectional ending but it is shortend in 
	//Afganistan, Uzbekistan, etc
	if((endIndex >= 7)&& endsWith(wordStr, _stAn, endIndex))
		endIndex-=4;
	for(i = startIndex; i< endIndex; i++){
		_wordBuffer[i-startIndex] = wordStr[i];
	}
	_wordBuffer[i-startIndex] = L'\0';

	Symbol base =  Symbol(_wordBuffer);
	return Symbol(_wordBuffer);
}



bool BaseFormMaker::startsWith(const wchar_t* word, Symbol start, size_t index){
	std::wstring pref =  start.to_string();
	if((wcslen(word)-index) < (pref.length()))
		return false;
	else{
		for(size_t i = 0; i<pref.length(); i++){
			if(pref.at(i) !=  word[i+index]){
				return false;
			}
		}
		return true;
	}
}

bool BaseFormMaker::endsWith(const wchar_t* word, Symbol end, size_t index){
	std::wstring suf =  end.to_string();
	size_t sufLen = suf.length();
	size_t wordLen = wcslen(word);
	if(index < sufLen)
		return false;
	else{
		for(size_t i = 0; i<sufLen; i++){
			if(suf.at(i) !=  word[index -sufLen+i]){
				return false;
			}
		}
		Symbol temp = Symbol(word);
		return true;
	}
}
