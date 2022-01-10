// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRUNER_POS_H
#define PRUNER_POS_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTrainer/LinkedTag.h"
#include "Generic/parse/ParserTrainer/VocabPruner.h"


class PrunerPOS {
private:
	static const float targetLoadingFactor;

	// just hashes on word symbol
 	struct HashKey {
        size_t operator()(const Symbol s) const {
            return s.hash_code();
        }
    };
    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
            return (s1 == s2);
        }
    };
    typedef serif::hash_map<Symbol, LinkedTag* , HashKey, EqualKey> Table;
    Table* table;
	int size;
public:
    PrunerPOS(UTF8InputStream& in, VocabPruner* vp);
	LinkedTag* lookup(Symbol word);
	void print(char *filename);
};

#endif
