// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_BWRULEDICTIONARY_H
#define AR_BWRULEDICTIONARY_H

#define MAX_RULES_PER_CATEGORY 1000
#define MAX_CATEGORIES 10000
#define MAX_RULES 10000

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/UTF8OutputStream.h"
#include <utility>
#include <boost/functional/hash.hpp>

class BWRuleDictionary {
public:
	static BWRuleDictionary * getInstance();
	static void destroy();

	int getFollowingPossibilities(const Symbol& position, const Symbol& category, Symbol *results);
	//size_t getPrecedingPossibilities(const Symbol& position, const Symbol& category, Symbol *results);
	
	bool isABPermitted(const Symbol& A,  const Symbol& B);
	bool isBCPermitted(const Symbol& B,  const Symbol& C);
	bool isACPermitted(const Symbol& A,  const Symbol& C);

	size_t getNRules(){return _n_rules;}
	void dump(UTF8OutputStream &uos);

protected:
	BWRuleDictionary();
	~BWRuleDictionary();

	static BWRuleDictionary * instance;	

	void initialize();
	void readBWRuleDictionaryFile(const char *rule_dict_file);
	void addRule(const Symbol& position1, const Symbol& category1, const Symbol& position2, const Symbol& category2);

private: 
	char _message[1000];
	wchar_t _scratch[1000];

	struct DoubleHashEntry {
		size_t n_exists;
		Symbol first_category;
		Symbol second_category;
	};
	struct DoubleHashKey {
		size_t operator()(const std::pair<Symbol, Symbol>& sp) const {
			return (boost::hash_value(sp));
		}
	};
	struct DoubleEqualKey {
		bool operator()(const std::pair<Symbol, Symbol>& sp1, const std::pair<Symbol, Symbol>& sp2) const {
			return (sp1 == sp2);
		}
	};
	typedef serif::hash_map<std::pair<Symbol, Symbol>, DoubleHashEntry, DoubleHashKey, DoubleEqualKey> DoubleHashMap;
    DoubleHashKey doubleHasher;
    DoubleEqualKey doubleEqTester;
	struct SingleHashEntry {
		Symbol preceding[MAX_RULES_PER_CATEGORY];
		Symbol following[MAX_RULES_PER_CATEGORY];
		int n_preceding;
		int n_following;
	};

	struct SingleHashKey {
		size_t operator()(const Symbol& s) const {
			return (s.hash_code());
		}
	};
	struct SingleEqualKey {
		bool operator()(const Symbol& s1, const Symbol& s2) const {
			return (s1==s2);
		}
	};
	//typedef hash_map<Symbol*, float, HashKey, EqualKey> Table;
	typedef serif::hash_map<Symbol, SingleHashEntry, SingleHashKey, SingleEqualKey> SingleHashMap;
    SingleHashKey singleHasher;
    SingleEqualKey singleEqTester;

	SingleHashMap* _A_map;
	SingleHashMap* _B_map;
	SingleHashMap* _C_map; 

	DoubleHashMap* _BC_map;
	DoubleHashMap* _AB_map;
	DoubleHashMap* _AC_map;

	size_t _n_rules;


	SingleHashMap* getAppropriateSingleMap(const Symbol& position);
	DoubleHashMap* getAppropriateDoubleMap(const Symbol& position1, const Symbol& position2);

};
#endif
