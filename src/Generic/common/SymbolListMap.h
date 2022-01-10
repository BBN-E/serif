// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_LIST_MAP_H
#define SYMBOL_LIST_MAP_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolArray.h"

// A mapping of symbol->symbol list.
// the last member of the list should always be null, so we
// know where to stop


class SymbolListMap {
private:
	static const float _target_loading_factor;
/*
	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };

    struct MapEntry {
        int numtypes;
        Symbol* types;
    };
*/

public:
	typedef SymbolToSymbolArrayMap Map;
//	typedef hash_map<Symbol, MapEntry, HashKey, EqualKey> Map;
private:
	Map* _map;

public:

    SymbolListMap(UTF8InputStream& in, bool multiwordsymbols = false);
	~SymbolListMap();

	const Symbol* lookup(const Symbol s, int& numTypes) const;
};
#endif
