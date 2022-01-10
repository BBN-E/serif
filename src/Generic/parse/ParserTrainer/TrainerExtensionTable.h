// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRAINER_EXTENSION_TABLE_H
#define TRAINER_EXTENSION_TABLE_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/ParserTrainer/LinkedBridgeExtension.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/common/Symbol.h"

class TrainerExtensionTable {
private:

	// same as for ExtensionTable
    static const float targetLoadingFactor;
    struct HashKey {
        size_t operator()(const ExtensionKey& e) const {
            return
                (e.branchingDirection << 10) ^
                (e.constituentCategory.hash_code() << 8) ^
                (e.headCategory.hash_code() << 6) ^
                (e.modifierBaseCategory.hash_code() << 4) ^
                (e.previousModifierCategory.hash_code() << 2) ^
                (e.modifierTag.hash_code());
        }
    };
    struct EqualKey {
        bool operator()(const ExtensionKey& e1, const ExtensionKey& e2) const {
            return
                (e1.branchingDirection == e2.branchingDirection) &&
                (e1.constituentCategory == e2.constituentCategory) &&
                (e1.headCategory == e2.headCategory) &&
                (e1.modifierBaseCategory == e2.modifierBaseCategory) &&
                (e1.previousModifierCategory == e2.previousModifierCategory) &&
                (e1.modifierTag == e2.modifierTag);
        }
    };

	// different
    typedef serif::hash_map<ExtensionKey, LinkedBridgeExtension* , HashKey, EqualKey> Table;
    typedef Table::iterator iterator;
    Table* table;
	int size;
public:
    TrainerExtensionTable(int init_size) {
		int numBuckets = static_cast<int>(init_size / targetLoadingFactor);
	    table = _new Table(numBuckets);
		size = 0;
	}
	void add(int direction, Symbol cc, Symbol hc, Symbol prev, Symbol mbc,
							Symbol mc, Symbol mcf, Symbol mt);
	bool compare_extension(BridgeExtension e, int direction, Symbol cc, 
		Symbol hc, Symbol prev, Symbol mbc,	Symbol mc, Symbol mcf, Symbol mt);
	void print(char *filename);
 
};

#endif
