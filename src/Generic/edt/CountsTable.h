// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COUNTSTABLE_H
#define COUNTSTABLE_H

#include "Generic/common/Symbol.h"
#include <iostream>

class CountsTable {
public: 
	struct SymbolListNode {
		Symbol symbol;
		int count;	
		SymbolListNode *next;
	};


//public:
	CountsTable();
	CountsTable(const CountsTable &other);
	~CountsTable();
	int operator[] (Symbol key);
	void add(Symbol key, int increment = 1);
	bool keyExists(Symbol key);
	void cleanup();
	
	void dump(std::ostream &out, int indent = 0);
	friend std::ostream &operator <<(std::ostream &out, CountsTable &it) { it.dump(out, 0); return out; }
	
	class iterator {
        friend class CountsTable;
    private:
        CountsTable &_table;
        SymbolListNode *_element;
    public:
//		iterator();
        iterator(CountsTable &table, SymbolListNode *element);
		iterator(const iterator& iter);
		//friend bool operator==(const iterator i, const iterator j) {return 
		friend bool operator!=(const iterator i, const iterator j) 
			{return (&i._table != &j._table) || (i._element != j._element);}

		//std::pair<const Symbol, int>& operator*();
		std::pair<const Symbol, int> value();
        iterator& operator++();
	};

    friend class iterator;

    iterator begin();
    iterator end();

private:
	int *findValue(Symbol key);
	void addKey(Symbol key, int initialValue = 0);
	
	SymbolListNode *copyNode(SymbolListNode *node);
	SymbolListNode *_head;
};

#endif
