// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRAINER_POS_H
#define TRAINER_POS_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTrainer/LinkedTag.h"

class TrainerPOS {
private:
	static const float targetLoadingFactor;

	// just hashes on word symbol
 	struct HashKey {
        size_t operator()(const Symbol* s) const {
            return s[0].hash_code();
        }
    };
    struct EqualKey {
        bool operator()(const Symbol* s1, const Symbol* s2) const {
            return ((s1[0] == s2[0]) && (s1[1] == s2[1]));
        }
    };
    typedef serif::hash_map<Symbol*, LinkedTag* , HashKey, EqualKey> Table;
    Table* table;
	int size;
public:
    TrainerPOS(int init_size);
	void add (Symbol* word, Symbol tag);
	void print(char *filename);
};

#endif
