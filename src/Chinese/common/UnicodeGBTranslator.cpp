// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/common/UnicodeGBTranslator.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include <fstream>

using namespace std;

const float UnicodeGBTranslator::targetLoadingFactor = static_cast<float>(0.7);
const Symbol UnicodeGBTranslator::NIL = Symbol();

UnicodeGBTranslator* UnicodeGBTranslator::_instance = 0;

UnicodeGBTranslator* UnicodeGBTranslator::getInstance() {
	if (_instance == 0)
		_instance = _new UnicodeGBTranslator();
	return _instance;
}

UnicodeGBTranslator::UnicodeGBTranslator() {

	// read encoding table
	std::string table_file = ParamReader::getRequiredParam("unicode_gb_table");
	readUnicodeGBTable(table_file.c_str());
	readGBUnicodeTable(table_file.c_str());
}

void UnicodeGBTranslator::readUnicodeGBTable(const char* tableFile) {
	int numEntries;
    int numBuckets;

	int key, val;
    wchar_t keyStr[2], valStr[2];
	keyStr[1] = valStr[1] = L'\0';

	basic_ifstream<char> in;
	in.open(tableFile);

    in >> numEntries;
	numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
	_unicode2gb = _new SymbolHash(numBuckets);

	for (int i = 0; i < numEntries; i++) {
		// Read in key, value
		in >> hex >> key;
        if (in.eof()) return;
		in >> hex >> val;

		// Translate value int to GB
		val += 0x8080;

		keyStr[0] = static_cast<wchar_t>(key);
		valStr[0] = static_cast<wchar_t>(val);
        (*_unicode2gb)[Symbol(keyStr)] = Symbol(valStr);
    }
	in.close();
}

void UnicodeGBTranslator::readGBUnicodeTable(const char* tableFile) {
	int numEntries;
    int numBuckets;

	int key, val;
    wchar_t keyStr[2], valStr[2];
	keyStr[1] = valStr[1] = L'\0';

	ifstream in;
	in.open(tableFile);

    in >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
	_gb2unicode = _new SymbolHash(numBuckets);

    for (int i = 0; i < numEntries; i++) {
		// Read in value, key
		in >> hex >> val;
        if (in.eof()) return;
		in >> hex >> key;

		// Translate key int to GB
		key += 0x8080;

		keyStr[0] = static_cast<wchar_t>(key);
		valStr[0] = static_cast<wchar_t>(val);
        (*_gb2unicode)[Symbol(keyStr)] = Symbol(valStr);
    }
	in.close();
}

const Symbol UnicodeGBTranslator::gb2UnicodeLookup(Symbol sym) const
{
    SymbolHash::iterator iter;

    iter = _gb2unicode->find(sym);
    if (iter == _gb2unicode->end()) {
		return NIL;
    }
    return (*iter).second;
}

const Symbol UnicodeGBTranslator::unicode2GBLookup(Symbol sym) const
{
    SymbolHash::iterator iter;

    iter = _unicode2gb->find(sym);
    if (iter == _unicode2gb->end()) {
        return NIL;
    }
    return (*iter).second;
}

wchar_t UnicodeGBTranslator::unicode2GB(const wchar_t u_ch) const {

	if (u_ch <= 0x007f) {
		return u_ch;
	} else {
		wchar_t wstr[2];
		wstr[0] = u_ch;
		wstr[1] = L'\0';

		Symbol symResult = unicode2GBLookup(Symbol(wstr));
		if (symResult != NIL)
			return (symResult.to_string())[0];
		else
			return 0x25A0;  // black square
	}
}

wchar_t UnicodeGBTranslator::gb2Unicode(const wchar_t gb_ch) const {

	if (gb_ch <= 0x007f) {
		return gb_ch;
	} else {
		wchar_t wstr[2];
		wstr[0] = gb_ch;
		wstr[1] = L'\0';

		Symbol symResult = gb2UnicodeLookup(Symbol(wstr));
		if (symResult != NIL)
			return (symResult.to_string())[0];
		else
			return 0x25A0;  // black square
	}
}

void UnicodeGBTranslator::gb2Unicode(const wchar_t *gb_str, wchar_t *u_str) const {
	int i = 0;
	while (true) {
		if (gb_str[i] == 0x0000) {
			u_str[i] = 0x0000;
			return;
		}
		u_str[i] = gb2Unicode(gb_str[i]);
        i++;
	}
}

void UnicodeGBTranslator::unicode2GB(const wchar_t *u_str, wchar_t *gb_str) const {
	int i = 0;
	while (true) {
		if (u_str[i] == 0x0000) {
			gb_str[i] = 0x0000;
			return;
		}
		gb_str[i] = unicode2GB(u_str[i]);
        i++;
	}
}

