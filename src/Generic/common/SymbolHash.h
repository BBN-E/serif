// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_HASH_H
#define SYMBOL_HASH_H

#include <cstddef>
#include "Generic/common/hash_set.h"
#include "Generic/common/Symbol.h"

class SymbolHash {
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

public:
    typedef hash_set<Symbol, HashKey, EqualKey> Table;
private:
    Table* table;

public:
    SymbolHash(int init_size);
	SymbolHash(const char* filename, bool use_case = false); 
	~SymbolHash();
    inline bool lookup(Symbol s) const { return table->exists(s); }
	void add(Symbol s);
	Table::iterator begin();
	Table::iterator end();
	void print(const char* filename);
	size_t size() const;
};

#endif
