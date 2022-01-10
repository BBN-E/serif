// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "CountTable.h"
#include "Generic/common/UTF8OutputStream.h"

CountTable::CountTable()
{
	_counts = _new HashMap(10000);
}

CountTable::~CountTable()
{
	HashMap::iterator iter;
	for (iter = _counts->begin(); iter != _counts->end(); ++iter) {
		delete (*iter).second;
	}
	delete _counts;
}

int CountTable::getCount(Symbol word) {
	HashMap::iterator iter;
	iter = _counts->find(word);
	if (iter == _counts->end()) {
		return -1;
	}
	return ((Counter*)(*iter).second)->getCount();
}

void CountTable::increment(Symbol word) {
	Counter * counter;
	HashMap::iterator iter;
	iter = _counts->find(word);
	if (iter == _counts->end()) {
		counter = _new Counter();
		(*_counts)[word] = counter;
	} else
		counter = (Counter*)(*iter).second;
	counter->increment();
}

void CountTable::pruneToThreshold(int threshold, const char * rare_words_file) {
	GrowableArray <Symbol> to_remove;
	HashMap::iterator iter;
	for (iter = _counts->begin(); iter != _counts->end(); ++iter) {
		Counter * counter = (Counter*)(*iter).second;
		int c = counter->getCount();
		if (c < threshold) {
			delete counter;
			to_remove.add((*iter).first);
		}
	}
	bool write_file = (rare_words_file != 0);
	UTF8OutputStream out;
	if (write_file) {
		out.open(rare_words_file);
		out << L"prune-threshold=" << threshold << L"\n";
	}
	for (int i = 0; i < to_remove.length(); i++) {
		if (write_file) {
			out << to_remove[i].to_string() << L"\n";
		}
		_counts->remove(to_remove[i]);
	}
	if (write_file) {
		out.close();
	}

	for (int i = 0; i < to_remove.length(); i++) {
		to_remove.removeLast();
	}
}
