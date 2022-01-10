// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_WORD_SET_H
#define EN_WORD_SET_H

#include "Generic/common/Symbol.h"

#define MAX_WORDSET_SIZE 1000

/**
 * An ordered set of symbols.
 *
 * @author David A. Herman
 */
class WordSet {
public:

	/// Constructs a new WordSet from the given initialization file.
	WordSet(const char *file_name, bool case_sensitive=true);

	/// Determines whether the given word exists in this WordSet.
	bool contains(Symbol word) const;
	bool contains(const wchar_t *word) const;

	/// Writes a description of the WordSet to the given output stream.
	void dump(std::ostream &out, int indent = 0) const;

	/// Returns the size of the word set.
	int size() const;

private:
	void insert(Symbol word);
	void insertAt(int pos, Symbol word);

	bool _case_sensitive;
	Symbol _words[MAX_WORDSET_SIZE];
	int _size;
};

#endif
