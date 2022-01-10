// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/CountsTable.h"


CountsTable::CountsTable() {
	_head = NULL;
}

CountsTable::CountsTable(const CountsTable &other) {
	SymbolListNode *iterator;
	iterator = _head = copyNode(other._head);
	while(iterator != NULL) {
		iterator->next = copyNode(iterator->next);
		iterator = iterator->next;
	}
}

CountsTable::~CountsTable() {
	cleanup();
}

void CountsTable::cleanup() {
	SymbolListNode *newHead;
	while(_head != NULL) {
		newHead = _head->next;
		delete _head;
		_head = newHead;
	}
}

void CountsTable::addKey(Symbol key, int initialValue) {
	SymbolListNode *newHead = _new SymbolListNode();
	newHead->count = initialValue;
	newHead->next = _head;
	newHead->symbol = key;

	_head = newHead;
}

void CountsTable::add(Symbol key, int increment) {
	int *value = findValue(key);
	if (value == NULL)
		addKey(key, increment);
	else *value += increment;
}

int *CountsTable::findValue(Symbol key) {
	SymbolListNode *iterator = _head;
	while (iterator != NULL && iterator->symbol != key) 
		iterator = iterator->next;
	if (iterator == NULL)
		return NULL;
	else return & (iterator->count);
}

bool CountsTable::keyExists(Symbol key) {
	return findValue(key) != NULL;
}

int CountsTable::operator [](Symbol key) {
	int *result = findValue(key);
	if (result == NULL)
		return 0;
	return *result;
}

void CountsTable::dump(std::ostream &out, int indent) {
	SymbolListNode *iterator = _head;

	while(iterator != NULL) {
		out << "(\"" << iterator->symbol.to_debug_string() << "\", " << iterator->count << ") ";
		iterator = iterator->next;
	}
}

CountsTable::SymbolListNode *CountsTable::copyNode(CountsTable::SymbolListNode *node) {
	SymbolListNode *newNode;
	if(node == NULL)
		return NULL;
	newNode = _new SymbolListNode();
	newNode->count = node->count;
	newNode->next = node->next;
	newNode->symbol = node->symbol;
	return newNode;
}


//CountsTable &
//SymbolListNode *_element;
//CountsTable::iterator::iterator() : {}
CountsTable::iterator::iterator(const iterator& other) : _table(other._table), _element(other._element) {}

CountsTable::iterator::iterator(CountsTable &table, SymbolListNode *element): _table(table), _element(element) {}

//std::pair<const Symbol, int>& CountsTable::iterator::operator*() {
std::pair<const Symbol, int> CountsTable::iterator::value() {
	std::pair<const Symbol, int> result(_element->symbol, _element->count);
	//result.first = _element->symbol;
	//result.second = _element->count;
	return result;
}

CountsTable::iterator& CountsTable::iterator::operator++() {
	_element = _element->next;
	return *this;
}

CountsTable::iterator CountsTable::begin() {
	return iterator(*this, _head);
}

CountsTable::iterator CountsTable::end() {
	return iterator(*this, 0);
}
