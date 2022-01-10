// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_SEPTGRAM_FEATURE_H
#define D_T_SEPTGRAM_FEATURE_H

#include "common/Symbol.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8OutputStream.h"
#include "common/UTF8Token.h"
#include "common/InternalInconsistencyException.h"
#include "discTagger/DTFeatureType.h"

#include "discTagger/DTFeature.h"

using namespace std;


/** DTWordFeature is a feature used to store a 5-gram of words. Each "word" can
  * come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first
  * Symbol should represent the "future" value of the feature.
  */

class DTSeptgramFeature : public DTFeature {
public:
	DTSeptgramFeature(const DTFeatureType *type, Symbol s1, Symbol s2, Symbol s3,
		Symbol s4, Symbol s5, Symbol s6, Symbol s7)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _symbol3(s3), _symbol4(s4)
		, _symbol5(s5), _symbol6(s6), _symbol7(s7)
	{
		if (s1.is_null() || s2.is_null() || s3.is_null() || s4.is_null() 
			|| s5.is_null() || s6.is_null() || s7.is_null()) {
			throw InternalInconsistencyException(
				"DTSeptgramFeature::DTSeptgramFeature()",
				"Attempt to create DTSeptgramFeature with null Symbol");
		}
	}

	virtual void deallocate() { delete this; }


	virtual bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTSeptgramFeature *otherFeature =
			static_cast<const DTSeptgramFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_symbol6 == otherFeature->_symbol6 &&
				_symbol7 == otherFeature->_symbol7);
	}

	virtual size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ _symbol5.hash_code() ^ _symbol6.hash_code() ^ _symbol7.hash_code();
	}

	virtual bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTSeptgramFeature *otherFeature =
			static_cast<const DTSeptgramFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_symbol6 == otherFeature->_symbol6 &&
				_symbol7 == otherFeature->_symbol7);
	}

	virtual size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ _symbol5.hash_code() ^ _symbol6.hash_code() ^ _symbol7.hash_code();
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
		str += _symbol7.to_string();
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
		str += _symbol7.to_string();
	}

	virtual void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string() << L" ";
		out << _symbol3.to_string() << L" " << _symbol4.to_string() << L" ";
		out << _symbol5.to_string() << L" " << _symbol6.to_string() << L" ";
		out << _symbol7.to_string();
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
		in >> token;
		_symbol7 = token.symValue();
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
	Symbol _symbol7;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTSeptgramFeature *_freeList;
#endif
};

#endif
