// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"


#include "UnicodeEucKrEncoder.h"
#include "common/limits.h"
#include "common/ParamReader.h"
#include "common/Symbol.h"
#include "common/UnexpectedInputException.h"
#include <fstream>

using namespace std;

const float UnicodeEucKrEncoder::targetLoadingFactor = static_cast<float>(0.7);
const Symbol UnicodeEucKrEncoder::NIL = Symbol();

UnicodeEucKrEncoder* UnicodeEucKrEncoder::_instance = 0;

UnicodeEucKrEncoder* UnicodeEucKrEncoder::getInstance() {
	if (_instance == 0)
		_instance = _new UnicodeEucKrEncoder();
	return _instance;
}

UnicodeEucKrEncoder::UnicodeEucKrEncoder() {

	// read encoding table
	char table_file[501];
	if (!ParamReader::getParam("unicode_euc_table",table_file,									 500))	{
		throw UnexpectedInputException("UnicodeEucKrEncoder::UnicodeEucKrEncoder()",
									   "Param `unicode_euc_table' not defined");
	}

	readUnicodeEUCTable(table_file);
	readEUCUnicodeTable(table_file);

}


void UnicodeEucKrEncoder::readUnicodeEUCTable(const char* tableFile) {
	int numEntries;
    int numBuckets;

	int key, val;
    wchar_t keyStr[2], valStr[2];
	keyStr[1] = valStr[1] = L'\0';

	basic_ifstream<unsigned char> in;
	in.open(tableFile);

    in >> numEntries;
	numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
	_unicode2euc = _new SymbolHash(numBuckets);

	for (int i = 0; i < numEntries; i++) {
		// Read in value, kry
		in >> hex >> val;
        if (in.eof()) return;
		in >> hex >> key;

		//ignore the rest of the line
		unsigned char line[200];
		in.getline(line, 200);

		// Translate value int to EUC-KR
		val += 0x8080;

		keyStr[0] = key;
		valStr[0] = val;
        (*_unicode2euc)[Symbol(keyStr)] = Symbol(valStr);
    }
	in.close();
}

void UnicodeEucKrEncoder::readEUCUnicodeTable(const char* tableFile) {
	int numEntries;
    int numBuckets;

	int key, val;
    wchar_t keyStr[2], valStr[2];
	keyStr[1] = valStr[1] = L'\0';

	ifstream in;
	in.open(tableFile);

    in >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
	_euc2unicode = _new SymbolHash(numBuckets);

    for (int i = 0; i < numEntries; i++) {
		// Read in key, value
		in >> hex >> key;
        if (in.eof()) return;
		in >> hex >> val;

		//ignore the rest of the line
		char line[200];
		in.getline(line, 200);

		// Translate key int to EUC-KR
		key += 0x8080;

		keyStr[0] = key;
		valStr[0] = val;
        (*_euc2unicode)[Symbol(keyStr)] = Symbol(valStr);
    }
	in.close();
}

const Symbol UnicodeEucKrEncoder::euc2UnicodeLookup(Symbol sym) const
{
    SymbolHash::iterator iter;

    iter = _euc2unicode->find(sym);
    if (iter == _euc2unicode->end()) {
		return NIL;
    }
    return (*iter).second;
}

const Symbol UnicodeEucKrEncoder::unicode2EUCLookup(Symbol sym) const
{
    SymbolHash::iterator iter;

    iter = _unicode2euc->find(sym);
    if (iter == _unicode2euc->end()) {
        return NIL;
    }
    return (*iter).second;
}

wchar_t UnicodeEucKrEncoder::unicode2EUC(const wchar_t u_ch) const {

	if (u_ch <= 0x007f) {
		return u_ch;
	} else {
		wchar_t wstr[2];
		wstr[0] = u_ch;
		wstr[1] = L'\0';

		Symbol symResult = unicode2EUCLookup(Symbol(wstr));
		if (symResult != NIL)
			return (symResult.to_string())[0];
		else
			return 0x25A0;  // black square
	}
}

wchar_t UnicodeEucKrEncoder::euc2Unicode(const wchar_t euc_ch) const {

	if (euc_ch <= 0x007f) {
		return euc_ch;
	} else {
		wchar_t wstr[2];
		wstr[0] = euc_ch;
		wstr[1] = L'\0';

		Symbol symResult = euc2UnicodeLookup(Symbol(wstr));
		if (symResult != NIL)
			return (symResult.to_string())[0];
		else
			return 0x25A0;  // black square
	}
}

void UnicodeEucKrEncoder::euc2Unicode(const wchar_t *euc_str, wchar_t *u_str, int max_len) const {
	int i = 0;
	while (true && (i < max_len - 1)) {
		if (euc_str[i] == L'\0') {
			u_str[i] = L'\0';
			return;
		}
		u_str[i++] = euc2Unicode(euc_str[i]);
	}
	u_str[i] = L'\0';
}

void UnicodeEucKrEncoder::euc2Unicode(unsigned char *euc_str, wchar_t *u_str, int max_len) const {
	wchar_t ch;
	int i = 0, j = 0;
	while (true && (i < max_len - 1)) {
		if (euc_str[j] == '\0') {
			break;
		}
		if (euc_str[j] <= 0x007f) {
			ch = static_cast<wchar_t>(euc_str[j]);
			j++;
		}
		else if (euc_str[j+1] != '\0') {
			ch = static_cast<wchar_t>((euc_str[j] << 8) + euc_str[j+1]); 
			j += 2;
		}
		else 
			throw UnexpectedInputException("UnicodeEucKrEncoder::euc2Unicode()",
											"unexpected EUC-KR string value");
		u_str[i++] = euc2Unicode(ch);
	}
	u_str[i] = L'\0';
}

std::wstring UnicodeEucKrEncoder::euc2Unicode(std::string euc_str) const {
	return euc2Unicode(convertToWideChars(euc_str));
}

std::wstring UnicodeEucKrEncoder::euc2Unicode(std::wstring euc_str) const {
	std::wstring result = L"";
	size_t len = euc_str.length();
	size_t i = 0;
	while (true && (i < len)) {
		if (euc_str.at(i) == L'\0') {
			break;
		}
		result.push_back(euc2Unicode(euc_str.at(i)));
		i++;
	}
	return result;
}

void UnicodeEucKrEncoder::unicode2EUC(const wchar_t *u_str, wchar_t *euc_str, int max_len) const {
	int i = 0;
	while (true && (i < max_len - 1)) {
		if (u_str[i] == L'\0') {
			break;
		}
		euc_str[i++] = unicode2EUC(u_str[i]);
	}
	euc_str[i] = L'\0';
}

void UnicodeEucKrEncoder::unicode2EUC(const wchar_t *u_str, unsigned char *euc_str, int max_len) const {
	unsigned char lowMask = 0xff;
	wchar_t ch;
	int i = 0, j = 0;
	while (true && (i < max_len - 2)) {
		if (u_str[j] == L'\0') {
			break;
		}
		if (u_str[j] <= 0x007f) {
			euc_str[i++] = static_cast<char>(u_str[j]);
		}
		else {
			ch = unicode2EUC(u_str[j]);
			euc_str[i++] = (ch >> 8);
			euc_str[i++] = (lowMask & ((unsigned char) ch)); 
		}
		j++;
	}
	euc_str[i] = '\0';
}

std::wstring UnicodeEucKrEncoder::unicode2EUC(std::wstring u_str) const {
	std::wstring result = L"";
	size_t len = u_str.length();
	size_t i = 0;
	while (true && (i < len)) {
		if (u_str.at(i) == L'\0') {
			break;
		}
		result.push_back(unicode2EUC(u_str.at(i)));
		i++;
	}
	return result;
}

void UnicodeEucKrEncoder::decomposeHangul(const wchar_t *str, 
										  wchar_t *hangul_str, 
										  int max_len) const
{
	long SBase = 0xAC00;
	long LBase = 0x1100;
	long VBase = 0x1161;
	long TBase = 0x11A7;
	long LCount = 19;
	long VCount = 21;
	long TCount = 28;
	long NCount = 588;	//VCount * TCount;
	long SCount = 11172; //LCount * NCount; 

	size_t orig_len = wcslen(str);

	size_t i = 0;
	int j = 0;
	while (i < orig_len && j < max_len - 3) {
		if (str[i] == L'\0') {
			break;
		}
		else if (str[i] <= 0x007f) {
			hangul_str[j++] = str[i];
		}
		else {
			long SIndex = static_cast<long>(str[i]) - SBase;
			if (SIndex < 0 || SIndex >= SCount) {
				hangul_str[j++] = str[i];
			}
			else {
				long L = LBase + (SIndex / NCount);
				long V = VBase + ((SIndex % NCount) / TCount);
				long T = TBase + (SIndex % TCount);
				hangul_str[j++] = static_cast<wchar_t>(L);
				hangul_str[j++] = static_cast<wchar_t>(V);
				if (T != TBase)
					hangul_str[j++] = static_cast<wchar_t>(T);
			}
		}
		i++;
	}
	hangul_str[j] = L'\0';
}

void UnicodeEucKrEncoder::decomposeHangul(const wchar_t *str, 
										  wchar_t *hangul_str, 
										  int *map,
										  int max_len) const
{
	long SBase = 0xAC00;
	long LBase = 0x1100;
	long VBase = 0x1161;
	long TBase = 0x11A7;
	long LCount = 19;
	long VCount = 21;
	long TCount = 28;
	long NCount = 588;	//VCount * TCount;
	long SCount = 11172; //LCount * NCount; 

	size_t orig_len = wcslen(str);

	int i = 0;
	int j = 0;
	while (i < static_cast<int>(orig_len) && j < max_len - 3) {
		if (str[i] == L'\0') {
			break;
		}
		else if (str[i] <= 0x007f) {
			map[j] = i;
			hangul_str[j++] = str[i];
		}
		else {
			long SIndex = static_cast<long>(str[i]) - SBase;
			if (SIndex < 0 || SIndex >= SCount) {
				map[j] = i;
				hangul_str[j++] = str[i];
			}
			else {
				long L = LBase + (SIndex / NCount);
				long V = VBase + ((SIndex % NCount) / TCount);
				long T = TBase + (SIndex % TCount);
				map[j] = i;
				hangul_str[j++] = static_cast<wchar_t>(L);
				map[j] = i;
				hangul_str[j++] = static_cast<wchar_t>(V);
				if (T != TBase) {
					map[j] = i;
					hangul_str[j++] = static_cast<wchar_t>(T);
				}
			}
		}
		i++;
	}
	map[j] = i;
	hangul_str[j] = L'\0';
}


std::wstring UnicodeEucKrEncoder::convertToWideChars(std::string str) const {
	std::wstring result;
	size_t len = str.length();
	size_t i = 0;

	while (i < len) {
		if (str.at(i) > 0 && str.at(i) <= 0x007f) {
			result.push_back(static_cast<wchar_t>(str.at(i)));
			i += 1;
		}
		else {
			wchar_t ch = (str.at(i) << 8); 
			ch += static_cast<unsigned char>(str.at(i+1));
			result.push_back(ch);	
			i += 2;
		}
	}
	return result;
}
