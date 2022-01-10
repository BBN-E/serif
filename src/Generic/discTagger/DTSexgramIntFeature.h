// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_SEXGRAM_FEATURE_INT_H
#define D_T_SEXGRAM_FEATURE_INT_H

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


/** DTWordFeature is a feature used to store a 5-gram of words plus an integer. 
  * Each "word" can come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first Symbol should 
  * represent the "future" value of the feature. Often the integer will be a 
  * word class index.
  */

class DTSexgramIntFeature : public DTFeature {
public:
	DTSexgramIntFeature(const DTFeatureType *type, Symbol s1, Symbol s2, Symbol s3,
		Symbol s4, Symbol s5, Symbol s6, int integer)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _symbol3(s3), 
		_symbol4(s4), _symbol5(s5), _symbol6(s6), _integer(integer)
	{
		if (s1.is_null() || s2.is_null() || s3.is_null() 
			|| s4.is_null() || s5.is_null() || s6.is_null()) {
			throw InternalInconsistencyException(
				"DTSexgramIntFeature::DTSexgramIntFeature()",
				"Attempt to create DTSexgramIntFeature with null Symbol");
		}
	}

	virtual void deallocate() { delete this; }


	virtual bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTSexgramIntFeature *otherFeature =
			static_cast<const DTSexgramIntFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_symbol6 == otherFeature->_symbol6 &&
				_integer == otherFeature->_integer);
	}

	virtual size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ _symbol5.hash_code() 
				^ _symbol6.hash_code() ^ ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	virtual bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTSexgramIntFeature *otherFeature =
			static_cast<const DTSexgramIntFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_symbol6 == otherFeature->_symbol6 &&
				_integer == otherFeature->_integer);
	}

	virtual size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			    _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ _symbol5.hash_code() 
				^ _symbol6.hash_code() ^ ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	virtual void toString(wstring &str) const {
		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += _symbol5.to_string();
		str += L" ";
		str += _symbol6.to_string();
		str += L" ";

		wchar_t buf[10];
		swprintf(buf, 10, L"%d", _integer);

		str += buf;
	}

	virtual void toStringWithoutTag(wstring &str) const {
		str = _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += _symbol5.to_string();
		str += L" ";
		str += _symbol6.to_string();
		str += L" ";

		wchar_t buf[10];
		swprintf(buf, 10, L"%d", _integer);

		str += buf;
	}

	virtual void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string() << L" ";
		out << _symbol3.to_string() << L" " << _symbol4.to_string() << L" ";
		out << _symbol5.to_string() << L" " << _symbol6.to_string() << L" " << _integer;
	}

	virtual void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
		in >> token;
		_symbol2 = token.symValue();
		in >> token;
		_symbol3 = token.symValue();
		in >> token;
		_symbol4 = token.symValue();
		in >> token;
		_symbol5 = token.symValue();
		in >> token;
		_symbol6 = token.symValue();
		in >> _integer;
	}

	virtual Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;
	Symbol _symbol3;
	Symbol _symbol4;
	Symbol _symbol5;
	Symbol _symbol6;
	int _integer;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTSexgramIntFeature *_freeList;
#endif
};

#endif
