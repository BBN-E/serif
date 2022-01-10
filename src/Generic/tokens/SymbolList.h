// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_LIST_H
#define SYMBOL_LIST_H

#include <iostream>
#include "Generic/common/Symbol.h"

/**
 * A bounded list of Symbols.
 *
 * @author David A. Herman
 */
class SymbolList {
public:

	/// Constructs a new SymbolList from the given initialization file.
	SymbolList(const char *file_name = 0);

	/// Indicates whether the given Symbol is in the list.
	/// This is O(N) in the length of the list.
	bool contains(Symbol sym) const;

	/// Writes a description of the list to the given output stream.
	void dump(std::ostream &out, int indent = 0) const;

	/// Returns the size of the list.
	int size() const;

	/// Returns the <code>i</code>th Symbol in the list.
	Symbol operator[](int i) const;

private:

	std::vector<Symbol> _symbols;
};

#endif
