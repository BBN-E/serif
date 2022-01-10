// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VOCABULARY_TABLE_H
#define VOCABULARY_TABLE_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_set.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/WordFeatures.h"

#include <cstddef>

class VocabularyTable {
private:
    static const float targetLoadingFactor;
	Symbol::HashSet *table;
	int size;
public:
    VocabularyTable(UTF8InputStream& in);
	VocabularyTable(UTF8InputStream& in, int threshold);
	VocabularyTable(int init_size) 
	{ 
		int numBuckets = static_cast<int>(init_size / targetLoadingFactor);
		table = _new Symbol::HashSet(numBuckets); 
	};
	void print(char* filename);
    bool find(Symbol word) const;
};

#endif
