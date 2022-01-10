// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XDOC_REL_FT_H
#define XDOC_REL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/DebugStream.h"
class DTState;

class XDocFT : public P1RelationFeatureType {
public:
	XDocFT() : P1RelationFeatureType(Symbol(L"from-xdoc")) {}

	void validateRequiredParameters() { initializeTable(); }

	virtual DTFeature *makeEmptyFeature() const;
	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const;
protected:
	void initializeTable();

	typedef	std::pair<const SymbolArray, const SymbolArray> KeyPair;

	struct Entry {
		int nTogether;
		Symbol relType;
		Entry *next;
	};

	struct EntryList {
		int n1;
		int n2;
		Entry *entry;
	};

    struct EqualPtr
    {
        bool operator()(const KeyPair &e1, const KeyPair &e2) const
        {
			return (e1.first == e2.first && e1.second == e2.second);
        }
    };

    struct HashPtr
    {
        size_t operator()(const KeyPair &p) const
        {
            return ((p.first.getHashCode() << 2) + p.second.getHashCode());
        }
    };

	typedef serif::hash_map<KeyPair, EntryList, HashPtr, EqualPtr> Table;
	Table *first_then_second;
	Table *second_then_first;

	DebugStream _debugOut;
};

#endif
