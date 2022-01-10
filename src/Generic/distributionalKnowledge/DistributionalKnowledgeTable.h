
#ifndef DISTRIBUTIONAL_KNOWLEDGE_TABLE_H
#define DISTRIBUTIONAL_KNOWLEDGE_TABLE_H

#include <set>
#include "Generic/common/hash_map.h"

class Symbol;


	
class ScoreEntry{
	public:
		static const int PMI = 0;
		static const int SIM = 1;
		static const int PRED_SUB = 2;
		static const int PRED_OBJ  = 3;
		static const int CAUSE = 2;

		float scores[4];

		ScoreEntry(){};
};


class DistributionalKnowledgeTable {
private:
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

	typedef std::pair<Symbol, Symbol> SymbolPair;
	typedef std::pair< std::set<int>, std::set<int> > IntsetPair;	// pair of integer set

	struct SymbolPairHash {
		size_t operator()(const SymbolPair& sp) const {
			size_t val = sp.first.hash_code();
			val = (val << 2) + sp.second.hash_code();
			return val;
		}
	};

	struct SymbolPairEqual {
		size_t operator()(const SymbolPair& sp1, const SymbolPair& sp2) const {
			return ((sp1.first==sp2.first) && (sp1.second==sp2.second)); 
		}
	};


	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> sSMap;			// symbol -> symbol
	typedef serif::hash_map<SymbolPair, Symbol, SymbolPairHash, SymbolPairEqual> spSMap;   // (symbol,symbol) -> symbol

	typedef serif::hash_map<SymbolPair, int, SymbolPairHash, SymbolPairEqual> spIMap;   // (symbol,symbol) -> int

	typedef serif::hash_map<Symbol, float, HashKey, EqualKey> sDMap;			// symbol -> float
	typedef serif::hash_map<SymbolPair, float, SymbolPairHash, SymbolPairEqual> spDMap;   // (symbol,symbol) -> float
	
	typedef serif::hash_map< Symbol, std::set<Symbol>, HashKey, EqualKey > sSetMap;	// symbol -> {Symbol}

	typedef serif::hash_map<SymbolPair, ScoreEntry, SymbolPairHash, SymbolPairEqual> spScoreEntryMap;   	// (symbol,symbol) -> array <float>

	typedef serif::hash_map<Symbol, IntsetPair, HashKey, EqualKey> sIspMap;		// symbol -> < {int}, {int} >
	

	static bool _is_initialized;

	static Symbol _verbSym;
	static Symbol _nounSym;
	static Symbol _adjSym;
	static Symbol _advSym;

	//Size of array in each entry
	static const int VN_ENTRY_SIZE  = 4;
	static const int VV_ENTRY_SIZE  = 3;
	static const int NN_ENTRY_SIZE  = 2;

	//How to handle different types of scores
	static const int NO_REORDER = 0;
	static const int REORDER = 1;

	static spScoreEntryMap* _vvScoreMap;
	static spScoreEntryMap* _nnScoreMap;
	static spScoreEntryMap* _vnScoreMap;
	static sIspMap* _nSubObjClusterMap;	// for each noun, its set of subject-cluster-ids and its set of object-cluster-ids


	// functions
	static void initPredicateSubObjClusters(const std::string& subFile, const std::string& objFile);
	static void initPredicateSubObjScores(const std::string& subFile, const std::string& objFile);
	static void readWWSimScores(const std::string& vnFile, const std::string& nnFile, const std::string& vvFile);
	static void readWWPmiScores(const std::string& vnFile, const std::string& nnFile, const std::string& vvFile);
	static void readCausalScores(const std::string& filename);
	
	//Generic Read/Lookup Methods for spScoreEntryMap
	static int readSymbolPairFloatTable(const std::string& fname, spScoreEntryMap* table, int entrysize, int entryindex,  int handle_order = NO_REORDER);
	static float lookupPairInMap(const Symbol& s1, const Symbol& s2, int entryindex, spScoreEntryMap* table, int handle_order = NO_REORDER);

public:
	static bool isInitialized() { return _is_initialized; }
	static void ensureInitialized();

	static float getCausalScore(const Symbol& v1, const Symbol& v2);
	static float getVNPmi(const Symbol& v, const Symbol& n);
	static float getNNPmi(const Symbol& n1, const Symbol& n2);
	static float getVVPmi(const Symbol& v1, const Symbol& v2);
	static float getVNSim(const Symbol& v, const Symbol& n);
	static float getNNSim(const Symbol& n1, const Symbol& n2);
	static float getVVSim(const Symbol& v1, const Symbol& v2);

	static float getPredicateSubScore(const Symbol& p, const Symbol& w);
	static float getPredicateObjScore(const Symbol& p, const Symbol& w);

	static std::set<int> getSubClusterIdsForArg(const Symbol& arg);
	static std::set<int> getObjClusterIdsForArg(const Symbol& arg);
};

#endif

