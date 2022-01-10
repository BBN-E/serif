// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include <wchar.h>
#include "Generic/common/limits.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/tokens/SymbolList.h"
#include <boost/scoped_ptr.hpp>

SymbolList::SymbolList(const char *file_name) {
	if (file_name == 0)
		return;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(file_name);

	std::wstring line;

	while (!in.eof()) {
		std::getline(in, line);
		if (line.size() > 0 && line[line.size() - 1] == '\r') // trim \r if it occurs
			line.resize(line.size() - 1);
		// If the line is blank, skip it.
		if (line.size() > 0) {
			_symbols.push_back(Symbol(line.c_str()));
		}
	}

	in.close();
}

bool SymbolList::contains(Symbol sym) const {
	return (std::find(_symbols.begin(), _symbols.end(), sym) != _symbols.end());
}

Symbol SymbolList::operator[](int i) const {
	return _symbols[i];
}

int SymbolList::size() const {
	return static_cast<int>(_symbols.size());
}

void SymbolList::dump(std::ostream &out, int indent) const {
	// TODO: indent
	out << '{';
	for (std::vector<Symbol>::size_type i = 0; i < _symbols.size(); i++) {
		if (i > 0) {
			out << ',';
		}
		out << '"' << _symbols[i].to_debug_string() << '"';
	}
	out << '}';
}
