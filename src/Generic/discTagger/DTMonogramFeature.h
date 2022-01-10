// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_MONOGRAM_FEATURE_H
#define D_T_MONOGRAM_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

using namespace std;


/** DTWordFeature is a feature used to store a monogram of words. Each "word" can
  * come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first
  * Symbol should represent the "future" value of the feature.
  */

class DTMonogramFeature : public DTFeature {
public:
	DTMonogramFeature(const DTFeatureType *type, Symbol s1)
		: DTFeature(type), _symbol1(s1)
	{
		if (s1.is_null()) {
			throw InternalInconsistencyException(
				"DTMonogramFeature::DTMonogramFeature()",
				"Attempt to create DTMonogramFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTMonogramFeature *otherFeature =
			static_cast<const DTMonogramFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1);
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code();
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		return true;
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code();
	}

	void toString(wstring &str) const {
		str = _symbol1.to_string();
	}

	void toStringWithoutTag(wstring &str) const {
		str = L"";
	}

	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string();
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
	}

	Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTMonogramFeature *_freeList;
#endif
};

#endif
