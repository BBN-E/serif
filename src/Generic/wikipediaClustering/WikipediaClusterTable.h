// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WIKIPEDIA_CLUSTER_TABLE_H
#define WIKIPEDIA_CLUSTER_TABLE_H

#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include <vector>

class WikipediaClusterTable {
private:
	static const float targetLoadingFactor;
	static const int numBuckets;	
	static void initPerplexityTable(const char *perplexity_file);
	static void initDistanceTables(const char *distance_file);
	
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

public:
	typedef hash_map<Symbol, std::vector<Symbol>, HashKey, EqualKey> ClusterTable;
	typedef hash_map<Symbol, float, HashKey, EqualKey> PerplexityTable;
	typedef hash_map<Symbol, int, HashKey, EqualKey> DistanceTable;
	typedef hash_map<Symbol, Symbol, HashKey, EqualKey> TypeTable;

	static void initTable(const char *bits_file,
						  const char *lc_bits_file = 0);
	static bool isInitialized() { return _is_initialized; }
	static void ensureInitializedFromParamFile();

	static std::vector<Symbol>* get(Symbol word, bool lowercase = false) { 
		if (lowercase && _lc_active)
			return _lowercaseTable.get(word); 
		else {
				
				return _table.get(word); 
		}
	}
	static float getPerplexity(Symbol path){
		if(_perplexTable.get(path) != NULL){
			return *_perplexTable.get(path);
		}
		else
			return 0;
	}
	static double getMaxPerplexity(){
		return _maxPerplexity;
	}
	
		static int getDistance(Symbol path){
		if (_has_distance){
			if(_distanceTable.get(path) != NULL){
				return *(_distanceTable.get(path));
			}
		}
		return NULL;
	}

	static Symbol getDistanceType(Symbol path){
		if (_has_distance){
			if(_typeTable.get(path) != NULL){
				return *(_typeTable.get(path));
			}
		}
		return Symbol(L"");
	}
	static Symbol getCenter(Symbol path){
		if (_has_distance){
			if(_centerTable.get(path) != NULL){
				return *(_centerTable.get(path));
			}
		}
		return Symbol(L"");
	}

private:	
    static ClusterTable _table;
    static ClusterTable _lowercaseTable;
	static PerplexityTable _perplexTable;
	static DistanceTable _distanceTable;
	static TypeTable _typeTable;
	static TypeTable _centerTable;
	static bool _is_initialized;
	static bool _lc_active;
	static bool _has_perplexities;
	static double _maxPerplexity;
	static bool _has_distance;

};

#endif

