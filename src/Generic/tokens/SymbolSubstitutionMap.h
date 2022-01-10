// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_SUBSTITUTION_MAP_H
#define SYMBOL_SUBSTITUTION_MAP_H

#include <iostream>
#include "Generic/common/Symbol.h"

class LocatedString;

#define MAX_SUBSTITUTION_MAP_SIZE 300

/**
 * A map of Symbol substitutions that should be made on a
 * token sequence as it is created by a tokenizer.
 *
 * @author David A. Herman
 */
class SymbolSubstitutionMap {
public:
	/// Constructs a new, empty SymbolSubstitutionMap.
	SymbolSubstitutionMap();

	/// Constructs a new SymbolSubstitutionMap from the given initialization file.
	SymbolSubstitutionMap(const char *file_name);

	/// Adds a symbol substitution pair to the map.
	void add(Symbol target, Symbol replacement);

	/// Indicates whether a given Symbol exists as a key in the map.
	bool contains(Symbol sym) const;

	/// Replaces the given Symbol with a Symbol from the substitution map.
	Symbol replace(Symbol sym) const;

	/// Replace all occurences of Symbols in the given LocatedString
	/// with corresponding Symbols from the substitution map.  (Modifies
	/// str.)
	void replaceAll(LocatedString &str) const;

	/// Replace all occurences of Symbols in the given string with
	/// corresponding Symbols from the substitution map.  (Modifies
	/// str.)
	void replaceAll(std::wstring &str) const;

	/// Replace all occurences of Symbols in the given LocatedString
	/// with reverse-corresponding Symbols from the substitution map.
	/// (Modifies str.)
	void reverseReplaceAll(LocatedString &str) const;

	/// Replace all occurences of Symbols in the given string with
	/// reverse-corresponding Symbols from the substitution map.
	/// (Modifies str.)
	void reverseReplaceAll(std::wstring &str) const;

	/// Get the size of the current map
	int getSize() const { return static_cast<int>(_map.size()); }

	/// Writes a description of the map to the given output stream.
	void dump(std::ostream &out, int indent = 0) const;

private:
	Symbol::HashMap<Symbol> _map;
};

#endif
