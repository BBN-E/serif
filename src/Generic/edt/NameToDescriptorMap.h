// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAMETODESCRIPTORMAP_H
#define NAMETODESCRIPTORMAP_H

#define MAX_LINKS 20

#include "common/Symbol.h"
#include "common/hash_map.h"
#include "common/UTF8InputStream.h"
#include <string.h>

using namespace std;

class NameToDescriptorMap
{
	struct WstringElement {
		wstring str;
		WstringElement *next;
	};

	static struct HashKey {
		size_t operator() (const wstring s) const {
			const wchar_t *str = s.c_str();
			size_t result = 0;
			while (*str != L'\0') {
				result = (result << 2) ^ *str++;
			}
			return result;
		}
	};

    static struct EqualKey {
        bool operator() (const wstring s1, const wstring s2) const {
			if (s1.length() != s2.length())
				return false;
			
			return !(s1.compare(s2));
        }
	}; 

	typedef hash_map <wstring, WstringElement *, HashKey, EqualKey> StringToStringMap;

private: 
	static StringToStringMap _headWordMap;
	static StringToStringMap _lastNameHeadWordMap;

	static bool _initialized;

	static wstring removeTitles(wstring name);
	static wstring safeToLower(wstring &str);

	static bool lookup(StringToStringMap &dataMap, wstring &key, wstring &value);
	static void addToMap(StringToStringMap &dataMap, wstring &key, wstring &value);

	static wstring removeFromEnd(wstring str, const wchar_t *suffix);
	static wstring findLastName(wstring name, bool strict);

public:
	static void initialize();
	static void destroy();

	static bool lookupNameHeadWordLink(wstring &name, wstring &headWord);
	static bool lookupLastNameHeadWordLink(wstring &name, wstring &head_word);
	static int getLastNameClusterAgreement(wstring &name, wstring &head_word);
	
};





#endif
