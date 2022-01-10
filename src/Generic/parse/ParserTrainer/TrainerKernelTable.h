// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRAINER_KERNEL_TABLE_H
#define TRAINER_KERNEL_TABLE_H

#include <cstddef>
#include "Generic/common/hash_map.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/ParserTrainer/LinkedBridgeKernel.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/common/Symbol.h"
//#include "Generic/common/UTF8InputStream.h"
//#include "Generic/common/UTF8OutputStream.h"

class TrainerKernelTable {
private:

	// same as for KernelTable
    static const float targetLoadingFactor;
    struct HashKey {
        size_t operator()(const KernelKey& k) const {
            return
                (k.branchingDirection << 6) ^
                (k.headBaseCategory.hash_code() << 4) ^
                (k.modifierBaseCategory.hash_code() << 2) ^
                (k.modifierTag.hash_code());
        }
    };
    struct EqualKey {
        bool operator()(const KernelKey& k1, const KernelKey& k2) const {
            return
                (k1.branchingDirection == k2.branchingDirection) &&
                (k1.headBaseCategory == k2.headBaseCategory) &&
                (k1.modifierBaseCategory == k2.modifierBaseCategory) &&
                (k1.modifierTag == k2.modifierTag);
        }
    };

	// different
    typedef serif::hash_map<KernelKey, LinkedBridgeKernel* , HashKey, EqualKey> Table;
    typedef Table::iterator iterator;
    Table* table;
	int size;
public:
    TrainerKernelTable(int init_size) {
		int numBuckets = static_cast<int>(init_size / targetLoadingFactor);
	    table = _new Table(numBuckets);
		size = 0;
	}
	void add(int direction, Symbol cc, Symbol hbc, Symbol mbc, Symbol hc,
							Symbol hcf, Symbol mc, Symbol mcf, Symbol mt);
	bool compare_kernel(BridgeKernel k, int direction, Symbol cc, Symbol hbc, 
		Symbol mbc, Symbol hc, Symbol hcf, Symbol mc, Symbol mcf, Symbol mt);
	void print(char *filename);
    
};

#endif
