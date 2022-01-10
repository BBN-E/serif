// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEQUENTIAL_BIGRAMS
#define SEQUENTIAL_BIGRAMS

#include "Generic/common/Symbol.h"

class SequentialBigrams {
private:
	Symbol* leftSequentialBigramCategories;
	int left_sequential_bigram_array_size;
	Symbol* rightSequentialBigramCategories;
	int right_sequential_bigram_array_size;
public:
	SequentialBigrams() : 
	  left_sequential_bigram_array_size(0),
	  right_sequential_bigram_array_size(0) {};
	SequentialBigrams(const char* filename);
	bool use_left_sequential_bigrams(Symbol category) const; 
	bool use_right_sequential_bigrams(Symbol category) const; 
};

#endif
