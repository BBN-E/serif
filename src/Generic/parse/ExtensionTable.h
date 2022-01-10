// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EXTENSION_TABLE_H
#define EXTENSION_TABLE_H

#include <fstream>
#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/common/Symbol.h"

class ExtensionTable {
private:
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
    struct ExtensionList {
        int length;
        BridgeExtension* data;
        ExtensionList(int _length, BridgeExtension* _data) :
            length(_length), data(_data)
        {}
        ExtensionList() {}
    };
public:
    ExtensionTable(UTF8InputStream& in);
	~ExtensionTable();
    typedef serif::hash_map<ExtensionKey, ExtensionList, HashKey, EqualKey> Table;
    typedef Table::iterator iterator;
    Table* table;
	const Table* getTable() { return table; }

    BridgeExtension* lookup(const ExtensionKey& key, int& numExtensions) const
    {
        iterator i = table->find(key);
        if (i == table->end()) {
            numExtensions = 0;
            return static_cast<BridgeExtension*>(0);
        } else {
            numExtensions = (*i).second.length;
            return (*i).second.data;
        }
    }
};

#endif
