// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_ARRAY_H
#define SYMBOL_ARRAY_H

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/hash_set.h"

class SymbolArray {
public:
	SymbolArray(): length(0), sarray(0) { }
	SymbolArray(const Symbol sarray_[], int length_) : length(0), sarray(0) {
		init(sarray_, static_cast<size_t>(length_));
	}

	SymbolArray(const Symbol sarray_[], size_t length_) : length(0), sarray(0) {
		init(sarray_, length_);
	}

	SymbolArray(const SymbolArray &other) : length(0), sarray(0) {
		init(other.sarray, other.length);
	}

	SymbolArray& operator=(const SymbolArray &other) {
		init(other.sarray, other.length);
		return *this;
	}

	bool operator==(const SymbolArray &other) const {
		if (length != other.length)
			return false;
		for (size_t i = 0; i < length; i++) {
			if (sarray[i] != other.sarray[i]) 
				return false;
		}
		return true;
	}

	inline bool operator!=(const SymbolArray &other) const { return !operator==(other); }


	void init(const Symbol sarray_[], size_t length_) {
		delete [] sarray;
		length = length_;
		sarray = _new Symbol[length];
		for(size_t i=0; i<length; i++){
			sarray[i] = sarray_[i];
		}
	}

	~SymbolArray() {
		delete[] sarray;
	}

	inline const Symbol* getArray() const { return sarray; }
	inline size_t getSizeTLength() const { return length; }
	inline int getLength() const { return static_cast<int>(length); }

	size_t getAltHashCode() const {
		size_t val = 0;
		for (size_t i = length; i >0; i--){
			val = (val << 3) - sarray[i-1].hash_code();
		}
		return val;
	}
	size_t getHashCode() const {
		size_t val = 0;
		for (size_t i = 0; i < length; i++){
			val = (val << 2) + sarray[i].hash_code();
		}
		return val;
	}
	friend struct SymbolArrayHashKey;
	friend struct SymbolArrayEqualKey;
protected:
	Symbol *sarray;
	size_t length;
};

struct SymbolArrayHashKey {
    size_t operator()(const SymbolArray *s) const {
        return s->getHashCode();
    }
};

struct SymbolArrayEqualKey {
    bool operator()(const SymbolArray *s1, const SymbolArray *s2) const {
		if(s1->length != s2->length)
			return false;

        for (size_t i = 0; i < s1->length; i++) {
            if (s1->sarray[i] != s2->sarray[i]) {
                return false;
            }
        }
        return true;
    }
}; 


typedef serif::hash_map<int, SymbolArray*, serif::IntegerHashKey, serif::IntegerEqualKey> IntegerSymbolArrayMap;
typedef serif::hash_map<SymbolArray*, SymbolArray*, SymbolArrayHashKey, SymbolArrayEqualKey> SymbolArraySymbolArrayMap;
typedef serif::hash_map<SymbolArray*, int, SymbolArrayHashKey, SymbolArrayEqualKey> SymbolArrayIntegerMap;
typedef serif::hash_map<Symbol, SymbolArray, Symbol::Hash, Symbol::Eq> SymbolToSymbolArrayMap;
typedef hash_set<SymbolArray*, SymbolArrayHashKey, SymbolArrayEqualKey> SymbolArraySet;

#endif
