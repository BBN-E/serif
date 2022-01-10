// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_BIGRAM_2_INT_FEATURE_H
#define D_T_BIGRAM_2_INT_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

using namespace std;


/** DTWordFeature is a feature used to store a trigram of words plus two integers. 
  * Each "word" can come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first Symbol should 
  * represent the "future" value of the feature. Often the integers will be 
  * word class indices.
  */

class DTBigram2IntFeature : public DTFeature {
public:
	DTBigram2IntFeature(const DTFeatureType *type, Symbol s1, Symbol s2, int integer1, int integer2)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _int1(integer1), _int2(integer2)
	{
		if (s1.is_null() || s2.is_null()) {
			throw InternalInconsistencyException(
				"DTBigram2IntFeature::DTBigram2IntFeature()",
				"Attempt to create DTBigram2IntFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigram2IntFeature *otherFeature =
			static_cast<const DTBigram2IntFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_int1 == otherFeature->_int1 &&
				_int2 == otherFeature->_int2);
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code()
			   ^ ((unsigned) _int1 << 3) ^ ((unsigned) _int2 << 3);
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigram2IntFeature *otherFeature =
			static_cast<const DTBigram2IntFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_int1 == otherFeature->_int1 &&
				_int2 == otherFeature->_int2);
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^ _symbol2.hash_code()
			   ^ ((unsigned) _int1 << 3) ^ ((unsigned) _int2 << 3);
	}

	void toString(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d", _int1, _int2);

		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
		str += L" ";
		str += buf;
	}

	void toStringWithoutTag(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d", _int1, _int2);

		str = _symbol2.to_string();
		str += L" ";
		str += buf;
	}


	void write(UTF8OutputStream &out) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d", _int1, _int2);

		out << _symbol1.to_string() << L" " << _symbol2.to_string() << L" ";
		out << buf;
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
		in >> token;
		_symbol2 = token.symValue();
		in >> _int1;		
		in >> _int2;
	}

	Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;

	int _int1;
	int _int2;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTBigram2IntFeature *_freeList;
#endif
};

#endif
