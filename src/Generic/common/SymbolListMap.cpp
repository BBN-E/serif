// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include "Generic/common/SymbolListMap.h"
#include "Generic/common/UnexpectedInputException.h"

const float SymbolListMap::_target_loading_factor = static_cast<float>(0.7);

SymbolListMap::SymbolListMap(UTF8InputStream& in, bool multiwordsymbols)
{
	int numBuckets = static_cast<int>(1000 / _target_loading_factor);

	if (numBuckets < 5)
		numBuckets = 5;
	_map = _new Map(numBuckets);

	UTF8Token token;
	Symbol word;
	int numtypes;
	wchar_t buffer[202];

	if (multiwordsymbols) {
		while (!in.eof()) {
			in >> token;
			if (in.eof())
				break;
			wcsncpy(buffer, token.chars(), 200);
			size_t len = wcslen(buffer);
			in >> token;
			while (wcscmp(token.chars(), L":STOP") != 0) {
				if (len < 200) {
					wcscat(buffer, L" ");
					wcsncat(buffer, token.chars(), 200-len);
					len = wcslen(buffer);
				}
				in >> token;
			}
			Symbol word(buffer);
			in >> numtypes;
			Symbol *newTypes = _new Symbol[numtypes];
			for (int j = 0; j < numtypes; j++) {
				in >> token;
				wcsncpy(buffer, token.chars(), 200);
				len = wcslen(buffer);
				in >> token;
				while (wcscmp(token.chars(), L":STOP") != 0) {
					if (len < 200) {
						wcscat(buffer, L" ");
						wcsncat(buffer, token.chars(), 200-len);
						len = wcslen(buffer);
					}
					in >> token;
				}
				newTypes[j] = Symbol(buffer);
			}
			(*_map)[word] = SymbolArray(newTypes, numtypes);
		}
	} else {
		while (!in.eof()) {
			in >> token;
			word = token.symValue();
			in >> numtypes;
			Symbol *newTypes = _new Symbol[numtypes];
			for (int j = 0; j < numtypes; j++) {
				in >> token;
				newTypes[j] = token.symValue();
			}
			(*_map)[word] = SymbolArray(newTypes, numtypes);
			// SymbolArray does not take the newTypes array, it makes a new one and fills it up
			// with the contents of newTypes. So newTypes must be deleted here.
			delete [] newTypes;
		}
	}
}


SymbolListMap::~SymbolListMap(){
	delete _map;
}


const Symbol* SymbolListMap::lookup(const Symbol word, int& numTypes) const
{
	Map::iterator iter;

	iter = _map->find(word);
	if (iter == _map->end()) {
		numTypes = 0;
		return static_cast<Symbol*>(0);
	}
	numTypes = (*iter).second.getLength();
	return (*iter).second.getArray();
}
