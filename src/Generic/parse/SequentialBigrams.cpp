// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/SequentialBigrams.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include <string>
#include <fstream>
#include <boost/scoped_ptr.hpp>

SequentialBigrams::SequentialBigrams(const char* filename) {

	boost::scoped_ptr<UTF8InputStream> bigram_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& bigram(*bigram_scoped_ptr);
	bigram.open(filename);

	Symbol cat;
	Symbol direction;
	UTF8Token token;
	int left_size = 0;
	int right_size = 0;

	while (!bigram.eof()) {
		bigram >> token;
		cat = token.symValue();
		if (cat == ParserTags::EOFToken)
			break;
		bigram >> token;
		direction = token.symValue();

		if (direction == ParserTags::LEFT) {
			left_size++;
		} else if (direction == ParserTags::RIGHT) {
			right_size++;
		} else if (direction == ParserTags::BOTH) {
			left_size++;
			right_size++;
		}
	}

	bigram.close();
	leftSequentialBigramCategories = _new Symbol[left_size];
	rightSequentialBigramCategories = _new Symbol[right_size];
	left_size = 0;
	right_size = 0;

	boost::scoped_ptr<UTF8InputStream> bigram2_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& bigram2(*bigram2_scoped_ptr);
	bigram2.open(filename);

	while (!bigram2.eof()) {
		bigram2 >> token;
		cat = token.symValue();
		if (cat == ParserTags::EOFToken)
			break;
		bigram2 >> token;
		direction = token.symValue();

		if (direction == ParserTags::LEFT) {
			leftSequentialBigramCategories[left_size] = cat;
			left_size++;
		} else if (direction == ParserTags::RIGHT) {
			rightSequentialBigramCategories[right_size] = cat;
			right_size++;
		} else if (direction == ParserTags::BOTH) {
			leftSequentialBigramCategories[left_size] = cat;
			left_size++;
			rightSequentialBigramCategories[right_size] = cat;
			right_size++;
		}
	}

	bigram2.close();

	left_sequential_bigram_array_size = left_size;
	right_sequential_bigram_array_size = right_size;

}

bool SequentialBigrams::use_left_sequential_bigrams(Symbol category) const {
	for (int i = 0; i < left_sequential_bigram_array_size; i++) {
		if (category == leftSequentialBigramCategories[i])
			return true;
	}
	return false;
}

bool SequentialBigrams::use_right_sequential_bigrams(Symbol category) const {
	for (int i = 0; i < right_sequential_bigram_array_size; i++) {
		if (category == rightSequentialBigramCategories[i])
			return true;
	}
	return false;
} 
