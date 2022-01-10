// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/common/InternalInconsistencyException.h"

#include <string.h>


ObjectIDTable::IDMap *ObjectIDTable::_table = 0;
int ObjectIDTable::_next_id;

void ObjectIDTable::initialize() {
	delete _table;
	_table = _new IDMap(OBJECT_ID_TABLE_SIZE);
	_next_id = 0;

	// make sure the null pointer is in there
	addObject(0);
}

void ObjectIDTable::finalize() {
	delete _table;
	_table = 0;
}

void ObjectIDTable::addObject(const void *object) {
	// make sure the object isn't already in the table.
/*	if (object != 0 && _table.get(object) != 0) {
		throw InternalInconsistencyException("ObjectIDTable::addObject()", "Request to add an object to the object ID table more than once.");
	}*/

	(*_table)[const_cast<void*>(object)] = _next_id++;
}

int ObjectIDTable::getID(const void *object) {
	int *result = _table->get(const_cast<void *>(object));
	if (result == 0) {
		throw InternalInconsistencyException("ObjectIDTable::getID()", "Request for the ID of an object that was never added to the table.");
	}
	return *result;
}
