// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_FEATURE_TABLE_H
#define MAXENT_FEATURE_TABLE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/DebugStream.h"
#include "Generic/discTagger/DTFeature.h"

class FeatureTableEntry {
public:
	FeatureTableEntry(DTFeature *feature) { _feature = feature; }

	int getID() { return _id; }
	void setID(int i) { _id = i; }
	DTFeature *getFeature() { return _feature; }

	size_t hash_value() const; 
	bool operator==(const FeatureTableEntry& f) const; 

private:
	DTFeature *_feature;
	int _id;
};

class MaxEntFeatureTable {
private:
	static const float targetLoadingFactor;

	// define hash_map mapping DTFeatures to floats
	struct FeatureHash {
        size_t operator()(const FeatureTableEntry *entry) const {
			return entry->hash_value();
        }
    };
    struct FeatureEquality {
        bool operator()(const FeatureTableEntry *f1, const FeatureTableEntry *f2) const {
            return (*f1) == (*f2);
        }
    };

	int _size;

public:
	typedef serif::hash_map<FeatureTableEntry *, double, FeatureHash, FeatureEquality> Table;
private:
	Table *_table;

public:
	MaxEntFeatureTable(int init_size);
	~MaxEntFeatureTable();

	double lookup(DTFeature *feature) const;
	int get_size() { return _size; }
	void add(DTFeature *feature);
	void add(DTFeature *feature, double value);
	void prune(int threshold);

	Table::iterator get_start() { return _table->begin(); }
	Table::iterator get_end() { return _table->end(); }
	Table::iterator get_element(DTFeature *feature);

	void print_to_open_stream(UTF8OutputStream& out);
};

#endif
