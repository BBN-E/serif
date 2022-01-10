// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KERNEL_TABLE_H
#define KERNEL_TABLE_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#if defined(_WIN32) || defined(__APPLE_CC__)
#include "Generic/common/hash_map.h"
#else
#include <ext/hash_map>
#endif
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/common/Symbol.h"

class KernelTable {
private:
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
    struct KernelList {
        int length;
        BridgeKernel* data;
        KernelList(int _length, BridgeKernel* _data) :
            length(_length), data(_data)
        {}
        KernelList() {}
    };
public:
    KernelTable(UTF8InputStream& in);
	~KernelTable();
#if defined(_WIN32) || defined(__APPLE_CC__)
    typedef serif::hash_map<KernelKey, KernelList, HashKey, EqualKey> Table;
#else
    typedef __gnu_cxx::hash_map<KernelKey, KernelList, HashKey, EqualKey> Table;
#endif
    typedef Table::iterator iterator;
    Table* table;
    BridgeKernel* lookup(const KernelKey& key, int& numKernels) const
    {
        iterator i = table->find(key);
        if (i == table->end()) {
            numKernels = 0;
            return static_cast<BridgeKernel*>(0);
        } else {
            numKernels = (*i).second.length;
            return (*i).second.data;
        }
    }
};

#endif
