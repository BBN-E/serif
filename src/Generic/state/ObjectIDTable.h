// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OBJECT_ID_TABLE_H
#define OBJECT_ID_TABLE_H

#include "Generic/common/hash_map.h"


#define OBJECT_ID_TABLE_SIZE 3


class ObjectIDTable {
public:
	// Call this before each use. It clears out the hash.
	static void initialize();

	// This is called optionally to deallocate the memory we used.
	// Afterward, ObjectIDTable may not be used, even if you call initialize()
	static void finalize();

	// This adds the object and assigns it a new ID.
	static void addObject(const void *object);

	// This retrieves the ID of the requested object. If it's not found,
	// it will raise an InternalInconsistency exception.
	static int getID(const void *object);

	static int getSize() { return _next_id; }


private:

	class PointerHash {
	public:
		size_t operator()(const void* p) const {
			return ((size_t) p) >> 2;
		}
	};
	class PointerEq {
	public:
		bool operator()(const void* p1, const void* p2) const {
            return p1 == p2;
        }
    };

	typedef serif::hash_map<void *, int, PointerHash, PointerEq> IDMap;

	static IDMap *_table;
	static int _next_id;
};

#endif
