// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORD_CLUSTER_TABLE_H
#define WORD_CLUSTER_TABLE_H

#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"

class WordClusterTable {
private:
	static const float targetLoadingFactor;
	static const int numBuckets;	

	struct HashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	typedef serif::hash_map<Symbol, int, HashKey, EqualKey> ClusterTable;
	

public:
	static bool isInitialized() { return _clusterTable._mixedcaseTable != NULL; }

	static void ensureInitializedFromParamFile();

	static int* get(Symbol word, bool lowercase = false) { 
		return _clusterTable.get(word, lowercase); }
	static int* domainGet(Symbol word, bool lowercase = false) {
		return _domainClusterTable.get(word, lowercase); }
	static int* secondaryGet(Symbol word, bool lowercase = false) {
		return _secondaryClusterTable.get(word, lowercase); }

private:
	struct ClusterTablePair {
		ClusterTable *_mixedcaseTable;
		ClusterTable *_lowercaseTable;
		ClusterTablePair(ClusterTable *mixedcaseTable=0, ClusterTable *lowercaseTable=0)
			: _mixedcaseTable(mixedcaseTable), _lowercaseTable(lowercaseTable) {}
		int* get(Symbol word, bool lowercase=false);
	};
	static ClusterTablePair initializeTable(const char *bits_file_param, const char *lc_bits_file_param);

	static ClusterTablePair _clusterTable;
	static ClusterTablePair _domainClusterTable;
	static ClusterTablePair _secondaryClusterTable;
};

#endif

