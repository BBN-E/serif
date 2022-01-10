// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include "Generic/common/SymbolHash.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnexpectedInputException.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

const float SymbolHash::targetLoadingFactor = static_cast<float>(0.7);

SymbolHash::SymbolHash(int init_size)
{
	int numBuckets = static_cast<int>(init_size / targetLoadingFactor);

	if (numBuckets < 5)
		numBuckets = 5;
	table = _new Table(numBuckets);
	
}
SymbolHash::~SymbolHash() {
	delete table;
}

void SymbolHash::add(Symbol s)
{
    table->insert(s);
}

SymbolHash::SymbolHash(const char* filename, bool use_case) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		std::string err = "Problem opening ";
		err.append(filename);
		throw UnexpectedInputException("SymbolHash::SymbolHash(const char* filename)",
			(char *)err.c_str());
	}

	table = _new Table(100);

	wchar_t line[501];
	while (!stream.eof()) {
		stream.getLine(line, 500);
		if (line[0] == L'#')
			continue;
		Symbol lineSym;
		if(use_case){
			lineSym = Symbol(line);
		}
		else{
			std::wstring lc_line(line);
			std::transform(lc_line.begin(), lc_line.end(), lc_line.begin(), towlower);
			lineSym = Symbol(lc_line.c_str());
		}
		add(lineSym);
	}

	stream.close();
}

SymbolHash::Table::iterator SymbolHash::begin() {
	return table->begin();
}

SymbolHash::Table::iterator SymbolHash::end() {
	return table->end();
}

void SymbolHash::print(const char* filename) {
	UTF8OutputStream stream(filename);

	Table::iterator iter;
	for (iter = table->begin(); iter != table->end(); ++iter) {
		Symbol sym = (*iter);
		stream << sym.to_string() << L"\n";
	}
	stream.close();
}

size_t SymbolHash::size() const {
	return table->size();
}
