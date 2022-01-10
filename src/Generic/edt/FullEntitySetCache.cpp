// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "edt/FullEntitySetCache.h"

FullEntitySetCache::FullEntitySetCache(int cache_size) {
	_keys = _new EntitySetUID[cache_size];
	_values = _new LexEntitySet*[cache_size];
	_capacity = cache_size;
	_nValues = 0;
}

FullEntitySetCache::~FullEntitySetCache() {
	cleanup();
	delete [] _keys;
	delete [] _values;
}

void FullEntitySetCache::loadPair(EntitySet *key, LexEntitySet *value) {
	setCapacity(_nValues+1);
	_values[_nValues] = value;
	_keys[_nValues] = (EntitySetUID) key;
	_nValues++;
}

LexEntitySet *FullEntitySetCache::retrieveData(const EntitySet *entitySet) {
	EntitySetUID uid = (EntitySetUID) entitySet;
	int i; 
	LexEntitySet *found = NULL;

	for(i=0; i<_nValues; i++) {
		if(_keys[i] == uid) {
			found = _values[i];
			_values[i] = NULL;
			break;
		}
	}
//	cleanup();
	return found;
}

void FullEntitySetCache::setCapacity(int capacity) {
	if(capacity > _capacity) {
		int new_capacity = capacity*2;
		EntitySetUID *newKeys = _new EntitySetUID[new_capacity];
		LexEntitySet **newValues = _new LexEntitySet*[new_capacity];

		int i;
		for(i=0; i<_nValues; i++) {
			newKeys[i] = _keys[i];
			newValues[i] = _values[i];
		}

		delete [] _keys;
		delete [] _values;
		_keys = newKeys;
		_values = newValues;
		_capacity = new_capacity;
	}
}

void FullEntitySetCache::cleanup() {
	int i;
	for(i=0; i<_nValues; i++) {
		delete _values[i];
		_keys[i] = 0;
	}
	_nValues = 0;
}
