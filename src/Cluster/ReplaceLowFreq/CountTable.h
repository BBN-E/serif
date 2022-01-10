#ifndef COUNT_TABLE_H
#define COUNT_TABLE_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Counter.h"


class CountTable
{
private:
	struct HashKey {
        size_t operator()(const Symbol key) const {
			return key.hash_code();
        }
    };
	struct EqualKey {
        bool operator()(const Symbol key1, const Symbol key2) const {
            return (key1 == key2);
        }
    };

public:
	typedef serif::hash_map<Symbol, Counter*, HashKey, EqualKey> HashMap;

public:
	CountTable(void);
	~CountTable(void);

	int getCount(Symbol word);
	void increment(Symbol word);
	void pruneToThreshold(int threshold, const char * rare_words_file = 0);

private:
	HashMap * _counts;

	size_t size() { return _counts->size(); }
	HashMap::iterator entries();
};


#endif
