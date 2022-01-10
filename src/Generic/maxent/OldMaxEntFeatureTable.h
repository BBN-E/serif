// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OLD_MAXENT_FEATURE_TABLE_H
#define OLD_MAXENT_FEATURE_TABLE_H
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolSet.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/DebugStream.h"

class Feature {
public:
	Feature() {};
	Feature(Symbol outcome, SymbolSet predicate);

	size_t hash_value() const; 
	bool operator==(const Feature& f) const; 

	SymbolSet& getPredicate() { return _predicate; }
	Symbol getOutcome() { return _outcome; }
	int getID() { return _id; }
	void setID(int i) { _id = i; }

	const SymbolSet EMPTY_PREDICATE;
	bool isPriorFeature() { return _predicate == EMPTY_PREDICATE; }
private:
	SymbolSet _predicate;
	Symbol _outcome;
	int _id;
};

class OldMaxEntFeatureTable {
private:
	static const float targetLoadingFactor;

	struct HashKey {
		size_t operator()(const Feature* f) const {
			return f->hash_value();
		}
	};

	struct EqualKey {
		bool operator()(const Feature* f1, const Feature* f2) const {
            return (*f1) == (*f2);
        }
	};

	int _size;

public:
	typedef serif::hash_map<Feature*, double, HashKey, EqualKey> Table;
private:
	Table *_table;

public:
	OldMaxEntFeatureTable(UTF8InputStream& stream);
	OldMaxEntFeatureTable(int init_size);
	~OldMaxEntFeatureTable();

	void print(const char *filename);
	void print_to_open_stream(UTF8OutputStream& out);
	void print_to_open_stream(DebugStream& out);
	double lookup(Symbol outcome, SymbolSet predicate) const;
	int get_size() { return _size; }
	void add(Symbol outcome, SymbolSet predicate);
	void add(Symbol outcome, SymbolSet predicate, double value);
	void prune(int threshold);

	Table::iterator get_start() { return _table->begin(); }
	Table::iterator get_end() { return _table->end(); }
	Table::iterator get_element(Symbol outcome, SymbolSet predicate);
};

#endif
