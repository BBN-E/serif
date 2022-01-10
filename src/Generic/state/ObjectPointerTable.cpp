// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/ObjectPointerTable.h"
#include "Generic/common/InternalInconsistencyException.h"


void ObjectPointerTable::initialize(int n_pointers) {
	_n_pointers = n_pointers;
	if (n_pointers > _size) {
		delete[] _pointers;
		_size = n_pointers;
		_pointers = _new void *[_size];
	}
    for (size_t i = 0; i < _n_pointers; i++){
		_pointers[i] = 0;
    }

	// pointer 0 is always a null pointer, just as in ObjectIDTable
	addPointer(0, 0);
}

ObjectPointerTable::~ObjectPointerTable() {
	delete[] _pointers;
}

void ObjectPointerTable::addPointer(int id, const void *pointer) {
	if ((unsigned) id >= (unsigned) _n_pointers) {
        std::stringstream err;
        err << "Request to add object with ID too high for table: table size = " << _n_pointers << "; ID = " << id;
		throw InternalInconsistencyException(
			"ObjectPointerTable::addPointer()", err.str().c_str());
	}
	_pointers[id] = const_cast<void*>(pointer);
}

void *ObjectPointerTable::getPointer(const void *id) {
	size_t id_int = reinterpret_cast<size_t>(id);
	if (id_int > _n_pointers) {
        std::stringstream err;
        err << "Request to get pointer with object ID too high for table: table size = " << _n_pointers << "; ID = " << id_int;
		throw InternalInconsistencyException(
			"ObjectPointerTable::getPointer()", err.str().c_str());
	}

	return _pointers[id_int];
}
