// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXDATACACHE_H
#define LEXDATACACHE_H

#include "edt/LexEntitySet.h"

class LexDataCache {
public:
	LexDataCache(int cache_size);
	~LexDataCache();
	LexEntitySet::LexData *retrieveData(const EntitySet *entitySet);
	void loadPair(EntitySet *key, LexEntitySet::LexData *value);
	void setCapacity(int capacity);
	void cleanup();

private:

	typedef EntitySet * EntitySetUID;
	EntitySetUID *_keys;
	LexEntitySet::LexData **_values;
	int _nValues, _capacity;
};

#endif
