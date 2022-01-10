// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OBJECT_POINTER_TABLE_H
#define OBJECT_POINTER_TABLE_H

#include <cstdlib>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED ObjectPointerTable {
public:
	
	ObjectPointerTable() : _size(0), _pointers(0), _n_pointers(0) {};
	~ObjectPointerTable();

	// Call this before each use and provide the number
	// of pointers you'll be adding. You may *not* add an object
	// whose ID is greater than or equal to that number, or
	// request the pointer of such an object (this will generate
	// an InternalInconsistencyException).
	void initialize(int n_pointers);

	// Associate id with given pointer
	void addPointer(int id, const void *pointer);

	// get pointer for given id
	// id is an int, but we take it as a void * becuase that's how
	// the objects have to store them.
	void *getPointer(const void *id);

	size_t getSize() { return _n_pointers; };

private:
	int _size;
	void **_pointers;
	size_t _n_pointers;
};

#endif
