// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FEATURE_VECTOR_H
#define FEATURE_VECTOR_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTrainer/LinkedTag.h"
#include "Generic/parse/WordFeatures.h"
#include "Generic/parse/VocabularyTable.h"
#include "Generic/parse/ParserTrainer/VocabPruner.h"
#include "Generic/common/UTF8InputStream.h"


class FeatureVectorTable {
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
    Table* _table;
	int _size;
	WordFeatures* _wordFeat;

public:
	FeatureVectorTable(UTF8InputStream& unprunedPOS, VocabPruner* pruner);
	LinkedTag* Lookup(Symbol word);
	void Add(Symbol feature, Symbol word);
	void Print(char *filename);
};

#endif
