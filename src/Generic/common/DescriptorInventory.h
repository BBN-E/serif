// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_INVENTORY_H
#define DESCRIPTOR_INVENTORY_H

#include <fstream>
#include "common/Symbol.h"
#include "common/hash_map.h"
#include "theories/EntityType.h"

class DescriptorInventory {
private:
	
	typedef struct {
		Symbol relType;
		EntityType otherEntityType;
	} RelationInventoryEntry;

	typedef struct {
		EntityType etype;
		RelationInventoryEntry *relations;
		int nrels;
	} InventoryEntry;

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

    HashKey hasher;
    EqualKey eqTester;

	typedef hash_map<Symbol, InventoryEntry *, HashKey, EqualKey> InventoryMap;

	InventoryMap *_inventoryMap;

	void throwFormatException(class Sexp *sexp);
public:
    
	DescriptorInventory(const char *filename);
	DescriptorInventory();
	void loadInventory(const char *filename);
	Symbol getDescType(Symbol word);
	Symbol getRelationType(Symbol stemmedPredicate, EntityType refType, EntityType otherType);
};

#endif



