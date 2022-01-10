// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ABBREVMAKER_H
#define ABBREVMAKER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/DebugStream.h"
#include <string.h>

class AbbrevMaker {
public:
	static const int MAX_ABBREVS = 16;

	static void loadData(UTF8InputStream &infile);
	/// SRS: Using this makes leak-detection easier
	static void unloadData();

	static int makeAbbrevs(Symbol word, Symbol results[], int max_results);
	static int restoreAbbrev(Symbol abbrev, Symbol results[], int max_results);
	
private:

	static int getNextWord(wchar_t buffer[], wchar_t word[], size_t max_word_length);
	static void getUntil(UTF8InputStream &in, wchar_t buffer[], int max_buffer_length, wchar_t delim);

    struct HashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	struct SymbolListNode {
		Symbol symbol;
		SymbolListNode *next;

		SymbolListNode() : next(0) {}
		~SymbolListNode() { delete next; }
	};

	typedef serif::hash_map <Symbol, SymbolListNode *, HashKey, EqualKey>
		SymbolToListMap;
	static SymbolToListMap *_abbrevMap, *_abbrevInverseMap;
	static DebugStream _debugOut;
};

#endif
