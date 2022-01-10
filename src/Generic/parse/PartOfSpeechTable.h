// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PART_OF_SPEECH_TABLE_H
#define PART_OF_SPEECH_TABLE_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"


class PartOfSpeechTable {
private:
    static const float targetLoadingFactor;
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
    struct TableEntry {
        int numTags;
        Symbol* tags;
    };
    typedef serif::hash_map<Symbol, TableEntry, HashKey, EqualKey> Table;
    Table* table;
public:
	PartOfSpeechTable(){
		table = _new Table(5);
	}
	~PartOfSpeechTable();
    PartOfSpeechTable(UTF8InputStream& in);
    const Symbol* lookup(Symbol word, int& numTags) const;
};

#endif
