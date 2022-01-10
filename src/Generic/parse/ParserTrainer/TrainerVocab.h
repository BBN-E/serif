// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRAINER_VOCAB_H
#define TRAINER_VOCAB_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"

class TrainerVocab {
private:
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
    typedef serif::hash_map<Symbol, int, HashKey, EqualKey> Table;
    Table* table;
	int size;
public:
	TrainerVocab(int NB);
	void print(char *filename);
	void add(Symbol s);
};

#endif
