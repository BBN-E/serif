// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FULL_ENTITY_SET_DATA_CACHE_H
#define FULL_ENTITY_SET_DATA_CACHE_H

#include "edt/LexEntitySet.h"

class FullEntitySetCache {
public:
	FullEntitySetCache(int cache_size);
	~FullEntitySetCache();
	LexEntitySet *retrieveData(const EntitySet *entitySet);
	void loadPair(EntitySet *key, LexEntitySet *value);
	void setCapacity(int capacity);
	void cleanup();

private:

	typedef EntitySet * EntitySetUID;
	EntitySetUID *_keys;
	LexEntitySet **_values;
	int _nValues, _capacity;
};

#endif
