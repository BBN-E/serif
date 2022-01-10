// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_QUADGRAM_2INT_FEATURE_H
#define D_T_QUADGRAM_2INT_FEATURE_H

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


/** DTQuadgramIntFeature is a feature used to store a 4-gram of words plus an integer. 
  * Each "word" can come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first Symbol should 
  * represent the "future" value of the feature. Often the integer will be a 
  * word class index.
  */

class DTQuadgram2IntFeature : public DTFeature {
public:
	DTQuadgram2IntFeature(const DTFeatureType *type, Symbol s1, Symbol s2, Symbol s3,
		Symbol s4, int integer1, int integer2)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _symbol3(s3), 
		_symbol4(s4), _integer1(integer1), _integer2(integer2)
	{
		if (s1.is_null() || s2.is_null() || s3.is_null() || s4.is_null()) {
			throw InternalInconsistencyException(
				"DTQuadgramIntFeature::DTQuadgram2IntFeature()",
				"Attempt to create DTQuadgramIntFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTQuadgram2IntFeature *otherFeature =
			static_cast<const DTQuadgram2IntFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_integer1 == otherFeature->_integer1 &&
				_integer2 == otherFeature->_integer2) ;
	}

	size_t getHashCode() const {
		return getFeatureType()->getName().hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ ((unsigned) _integer1 << 3) 
				^ ((unsigned) _integer2 << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTQuadgram2IntFeature *otherFeature =
			static_cast<const DTQuadgram2IntFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_integer1 == otherFeature->_integer1 &&
				_integer2 == otherFeature->_integer2) ;
	}

	size_t getHashCodeWithoutTag() const {
		return getFeatureType()->getName().hash_code() ^
			   _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ ((unsigned) _integer1 << 3) 
				^ ((unsigned) _integer2 << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	void toString(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d", _integer1, _integer2);
		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += buf;

	}

	void toStringWithoutTag(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d", _integer1, _integer2);
		str = _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += buf;

	}

	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string() << L" ";
		out << _symbol3.to_string() << L" " << _symbol4.to_string() << L" ";
		out << L" " << _integer1 << L" "<< _integer2;
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
		in >> token;
		_symbol2 = token.symValue();
		in >> token;
		_symbol3 = token.symValue();
		in >> token;
		_symbol4 = token.symValue();
		in >> _integer1;
		in >> _integer2;
	}

	Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;
	Symbol _symbol3;
	Symbol _symbol4;
	int _integer1;
	int _integer2;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTQuadgram2IntFeature *_freeList;
#endif
};

#endif
