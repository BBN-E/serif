// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include <cstring>
#include "Generic/common/limits.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/common/LocatedString.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/scoped_ptr.hpp>

SymbolSubstitutionMap::SymbolSubstitutionMap() {
}

/**
 * @param file_name the file name of the initialization file.
 */
SymbolSubstitutionMap::SymbolSubstitutionMap(const char *file_name) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(file_name);

	std::wstring old_source;
	std::wstring new_source;
	while (!in.eof()) {
		std::getline(in, old_source);
		if (old_source.size() == 0)
			continue;
		if (old_source[old_source.size() - 1] == '\r') // trim \r if it occurs
			old_source.resize(old_source.size() - 1);

		if (!in.eof()) {
			std::getline(in, new_source);
			if (new_source.size() == 0) {
				std::stringstream ss;
				ss << "Unexpected blank line in: " << file_name;
				throw UnexpectedInputException("SymbolSubstitutionMap::SymbolSubstitutionMap",
											   ss.str().c_str());
			}
			if (new_source[new_source.size() - 1] == '\r')
				new_source.resize(new_source.size() - 1);

			add(Symbol(old_source.c_str()), Symbol(new_source.c_str()));
		}
		else {
			std::stringstream ss;
			ss << "No replacement string specified for last entry in: " << file_name;
			in.close();
			throw UnexpectedInputException("SymbolSubstitutionMap::SymbolSubstitutionMap",
										   ss.str().c_str());
		}
	}

	in.close();
}

/**
 * @param target the symbol to be replaced.
 * @param replacement the symbol to replace it with.
 */
void SymbolSubstitutionMap::add(Symbol target, Symbol replacement) {
	if (contains(target)) {
		SessionLogger::warn("symbol_substitution") 
			<< "Multiple substitution replacements specified for "
			<< "a single target: [" << target.to_debug_string() << "]";
		return;
	}
	_map[target] = replacement;
}

/**
 * @param sym the Symbol to search for.
 * @return <code>true</code> if the given Symbol exists as a key within
 *         the map; <code>false</code> if not.
 */
bool SymbolSubstitutionMap::contains(Symbol sym) const {
	return (_map.find(sym) != _map.end());
}

/**
 * If the symbol is not found in the substitution map, it is
 * returned unchanged.
 *
 * @param sym the symbol to substitute.
 * @return the substituted symbol.
 */
Symbol SymbolSubstitutionMap::replace(Symbol sym) const {
	Symbol::HashMap<Symbol>::const_iterator it = _map.find(sym);
	if (it == _map.end())
		return sym;
	else
		return (*it).second;
}

void SymbolSubstitutionMap::replaceAll(LocatedString &str) const {
	typedef Symbol::HashMap<Symbol>::const_iterator MapIter;
	for (MapIter it = _map.begin(); it != _map.end(); ++it)
		str.replace((*it).first.to_string(), (*it).second.to_string());
}

void SymbolSubstitutionMap::replaceAll(std::wstring &str) const {
	typedef Symbol::HashMap<Symbol>::const_iterator MapIter;
	for (MapIter it = _map.begin(); it != _map.end(); ++it)
		boost::replace_all(str, (*it).first.to_string(), (*it).second.to_string());
}

void SymbolSubstitutionMap::reverseReplaceAll(LocatedString &str) const {
	typedef Symbol::HashMap<Symbol>::const_iterator MapIter;
	for (MapIter it = _map.begin(); it != _map.end(); ++it)
		str.replace((*it).second.to_string(), (*it).first.to_string());
}

void SymbolSubstitutionMap::reverseReplaceAll(std::wstring &str) const {
	typedef Symbol::HashMap<Symbol>::const_iterator MapIter;
	for (MapIter it = _map.begin(); it != _map.end(); ++it)
		boost::replace_all(str, (*it).second.to_string(), (*it).first.to_string());
}


/**
 * @param out the output stream to write to.
 * @param indent the number of characters to indent.
 */
void SymbolSubstitutionMap::dump(std::ostream &out, int indent) const {
	// Indent.
	for (int i = 0; i < indent; i++) {
		out << ' ';
	}

	bool first_item = true;
	typedef Symbol::HashMap<Symbol>::const_iterator MapIter;
	for (MapIter it = _map.begin(); it != _map.end(); ++it) {
		if (!first_item) out << ',';
		out << "\""
			<< (*it).first.to_debug_string()
			<< "\" => \""
			<< (*it).second.to_debug_string()
			<< "\"";
		first_item = false;
	}
	out << '}';
}
