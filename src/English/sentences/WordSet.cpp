// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "English/sentences/WordSet.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;

/**
 * @param file_name the file name of the initialization file.
 */
WordSet::WordSet(const char *file_name, bool case_sensitive) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(file_name);

	std::wstring source;

	_case_sensitive = case_sensitive;

	_size = 0;
	size_t i = 0;
	while ((i < MAX_WORDSET_SIZE) && !in.eof()) {
		in.getLine(source);
		if (source.size() == 0)
			continue;
		size_t len = source.length();		

		if (source.at(len - 1) == L'\r') {
			source.at(len - 1) = L'\0';
		}
		if (source.at(0) != L'\0') {
			if (!case_sensitive) {
				std::transform(source.begin(), source.end(), source.begin(), towlower);
			}
			insert(Symbol(source.c_str()));
			i++;
		}
	}

	if (!in.eof()) {
		// Get the default session logger.
		SessionLogger::warn("word_set_limit") << "WordSet exceeds entry limit of "
				<< MAX_WORDSET_SIZE << "\n";
	}

	in.close();
}


void WordSet::insert(Symbol word) {
	int i = 0;
	while ((i < _size) && (wcscmp(word.to_string(), _words[i].to_string()) > 0)) {
		i++;
	}
	// Don't reinsert it if it's already there.
	if (word == _words[i]) {
		return;
	}
	insertAt(i, word);
}

void WordSet::insertAt(int pos, Symbol word) {
	for (int i = _size; i > pos; i--) {
		_words[i] = _words[i - 1];
	}
	_words[pos] = word;
	_size++;
}

bool WordSet::contains(Symbol word) const {
	if (!_case_sensitive) {
		std::wstring word_str(word.to_string());
		std::transform(word_str.begin(), word_str.end(), word_str.begin(), towlower);
		word = Symbol(word_str.c_str());
	}

	// Your basic binary search.
	int lo = 0;
	int hi = _size - 1;
	int mid;

	bool ret = false;

	while (lo <= hi) {
		mid = (lo + hi) / 2;
		if (word == _words[mid]) {
			return true;
		}
		else {
			int cmp = wcscmp(word.to_string(), _words[mid].to_string());
			if (cmp > 0) {
				lo = mid + 1;
			}
			else {
				hi = mid - 1;
			}
		}
	}
	return false;
}


bool  WordSet::contains(const wchar_t *word) const  {

	std::wstring word_str(word);

	if (!_case_sensitive) {
		std::transform(word_str.begin(), word_str.end(), word_str.begin(), towlower);
	}

	// Your basic binary search.
	int lo = 0;
	int hi = _size - 1;
	int mid;

	bool ret = false;

	while (lo <= hi) {
		mid = (lo + hi) / 2;
		if (!wcscmp(word_str.c_str(), _words[mid].to_string())) {
			return true;
		}
		else {
			int cmp = wcscmp(word_str.c_str(), _words[mid].to_string());
			if (cmp > 0) {
				lo = mid + 1;
			}
			else {
				hi = mid - 1;
			}
		}
	}

	return false;
}


int WordSet::size() const {
	return _size;
}

/**
 * @param out the output stream to write to.
 * @param indent the number of characters to indent.
 */
void WordSet::dump(std::ostream &out, int indent) const {
	// TODO: indent
	for (int i = 0; i < _size; i++) {
		out << "\""
			<< _words[i].to_debug_string()
			<< "\""
			<< endl;
	}
}
