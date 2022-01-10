// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_BIGRAM_FEATURE_H
#define D_T_BIGRAM_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

using namespace std;


/** DTWordFeature is a feature used to store a Bigram of words. Each "word" can
  * come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first
  * Symbol should represent the "future" value of the feature.
  */

class DTBigramFeature : public DTFeature {
public:
	DTBigramFeature(const DTFeatureType *type, Symbol s1, Symbol s2)
		: DTFeature(type), _symbol1(s1), _symbol2(s2)
	{
		if (s1.is_null() || s2.is_null()) {
			throw InternalInconsistencyException(
				"DTBigramFeature::DTBigramFeature()",
				"Attempt to create DTBigramFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigramFeature *otherFeature =
			static_cast<const DTBigramFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2);
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code();
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigramFeature *otherFeature =
			static_cast<const DTBigramFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2);
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^ _symbol2.hash_code();
	}

	void toString(wstring &str) const {
		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
	}

	void toStringWithoutTag(wstring &str) const {
		str = _symbol2.to_string();
	}

	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string();
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
		in >> token;
		_symbol2 = token.symValue();
	}

	Symbol getTag() const {
		return _symbol1;
	}

	Symbol getNonTag() const {
	   return _symbol2;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTBigramFeature *_freeList;
#endif
};

#endif
