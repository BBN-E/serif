// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_PATTERN_SET_H
#define EVENT_PATTERN_SET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"

class EventPatternNode;
class Sexp;

class EventPatternSet {
public:
	EventPatternSet(const char* filename);
	~EventPatternSet();

private:
	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };

public:
    struct NodeSet {
        int n_nodes;
        EventPatternNode** nodes;
    };

	typedef serif::hash_map<Symbol, NodeSet, HashKey, EqualKey> Map;
private:
	Map* _map;
public:
	EventPatternNode** getNodes(const Symbol s, int& n_nodes) const;

private:
	void verifyEntityTypeSet(Sexp &sexp);

};

#endif
