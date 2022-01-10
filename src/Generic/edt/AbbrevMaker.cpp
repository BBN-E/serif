// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/AbbrevMaker.h"
#include "Generic/common/hash_map.h"
#include <iostream>


using namespace std;


DebugStream AbbrevMaker::_debugOut;
AbbrevMaker::SymbolToListMap * AbbrevMaker::_abbrevInverseMap;
AbbrevMaker::SymbolToListMap * AbbrevMaker::_abbrevMap;

void AbbrevMaker::loadData(UTF8InputStream &infile) {
	wchar_t buffer[256], word[256], abbrev[256];
	_abbrevMap = _new SymbolToListMap (400);
	_abbrevInverseMap = _new SymbolToListMap(500);
	Symbol key, abbrevSymbol;
	SymbolListNode *value;
	_debugOut.init(Symbol(L"abbrev_maker_debug_file"), false);
	_debugOut << "entered loadData()";
	while (!infile.eof()) {
		getUntil(infile, buffer, 256, L'(');
		_debugOut << "got1: " << buffer;
		getUntil(infile, buffer, 256, L')');
		_debugOut << "got2: " << buffer << "\n";
		size_t i = 0, inc = 0;
		i += getNextWord(&buffer[i], word, wcslen(buffer)-i);
		key = Symbol(word);
		inc = getNextWord(&buffer[i], abbrev, wcslen(buffer)-i);
		i += inc;
		while (inc > 0) {	
			//insert new key-value pairs in both tables
			abbrevSymbol = Symbol(abbrev);
			value = _new SymbolListNode();
			value->symbol = abbrevSymbol;
			//NOTE: here we assume that the default value for a pointer is 0
			value->next = (*_abbrevMap)[key];
			(*_abbrevMap)[key] = value;
			
			if(strcmp(abbrevSymbol.to_debug_string(), "yugoslav") == 0) {
				Symbol temporary = Symbol(L"african");
				//i = 0;
			}

			value = _new SymbolListNode();
			value->symbol = key;
			value->next = (*_abbrevInverseMap)[abbrevSymbol];
			(*_abbrevInverseMap)[abbrevSymbol] = value;
			inc = getNextWord(&buffer[i], abbrev, wcslen(buffer)-i);
			i += inc;
			_debugOut << "AbbrevMaker: processed [" << key.to_debug_string() << ", " << abbrevSymbol.to_debug_string() << "] \n";
		}
	}
	_debugOut << "done!!\n";
	_debugOut << "TEST:\n";
	Symbol results[16];
	Symbol test = Symbol(L"britain");
	int nResults = makeAbbrevs(test, results, 16);
	_debugOut << "Britain: " << nResults << " ";
	for (int k=0; k<nResults; k++)
		_debugOut << results[k].to_debug_string() << " ";
}

void AbbrevMaker::unloadData() {
	for (SymbolToListMap::iterator iter = _abbrevMap->begin();
		 iter != _abbrevMap->end();
		 ++iter)
	{
		delete (*iter).second;
	}
	delete _abbrevMap;

	for (SymbolToListMap::iterator it = _abbrevInverseMap->begin();
		it != _abbrevInverseMap->end();
		++it)
	{
		delete (*it).second;
	}
	delete _abbrevInverseMap;
}


void AbbrevMaker::getUntil(UTF8InputStream &in, wchar_t buffer[], int max_buffer_length, wchar_t delim) {
	int i = 0;
	for(i=0; i<max_buffer_length-1; i++) 
		if((buffer[i] = in.get()) == delim) 
			break;
	buffer[i] = L'\0';

}

int AbbrevMaker::getNextWord(wchar_t buffer[], wchar_t word[], size_t max_word_length) {
	int i=0;
	while(buffer[i] != L' ' && buffer[i] != 0) {
		word[i] = buffer[i];
		i++;
	}
	word[i] = L'\0';
	if(buffer[i] == L' ') i++;
	return i;
}

int AbbrevMaker::makeAbbrevs(Symbol word, Symbol results[], int max_results) {
	int i;
	SymbolListNode *iterator;
	iterator = (*_abbrevMap)[word];
	i = 0;
	while (iterator != NULL && i < max_results) {
		results[i] = iterator->symbol;
		iterator = iterator->next;
		i++;
	}
	return i;
}

int AbbrevMaker::restoreAbbrev(Symbol abbrev, Symbol results[], int max_results) {
	int i;
	SymbolListNode *iterator;
	if(strcmp(abbrev.to_debug_string(), "yugoslav") == 0) {
		Symbol temporary = Symbol(L"african");
		i = 0;
	}
	iterator = (*_abbrevInverseMap)[abbrev];
	i = 0;
	while (iterator != NULL && i < max_results) {
		results[i] = iterator->symbol;
		iterator = iterator->next;
		i++;
	}
	return i;
}
