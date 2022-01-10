// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_BIGRAM_INT_FEATURE_H
#define D_T_BIGRAM_INT_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

using namespace std;


/** DTBigramIntFeature is a feature used to store a bigram of words plus an integer. 
  * Each "word" can come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first
  * Symbol should represent the "future" value of the feature. Often the integer will be a 
  * word class index.
  */

class DTBigramIntFeature : public DTFeature {
public:
	DTBigramIntFeature(const DTFeatureType *type, Symbol s1, Symbol s2, int integer)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _integer(integer)
	{
		if (s1.is_null() || s2.is_null()) {
			throw InternalInconsistencyException(
				"DTBigramIntFeature::DTBigramIntFeature()",
				"Attempt to create DTBigramIntFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigramIntFeature *otherFeature =
			static_cast<const DTBigramIntFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_integer == otherFeature->_integer);
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code() ^ ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTBigramIntFeature *otherFeature =
			static_cast<const DTBigramIntFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_integer == otherFeature->_integer);
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol2.hash_code() ^ ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	void toString(wstring &str) const {
		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
		str += L" ";

		wchar_t buf[10];
		swprintf(buf, 10, L"%d", _integer);

		str += buf;
	}

	void toStringWithoutTag(wstring &str) const {
		str = _symbol2.to_string();
		str += L" ";

		wchar_t buf[10];
		swprintf(buf, 10, L"%d", _integer);

		str += buf;
	}

	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string()
			<< L" " << _integer;
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_symbol1 = token.symValue();
		in >> token;
		_symbol2 = token.symValue();
		in >> _integer;
		if (in.bad()) {
			in >> token;
			in.clear();
			throw UnexpectedInputException("DTBigramIntFeature::read()", 
										   "expected an int, got: ",
										   token.symValue().to_debug_string());
		}
			
	}

	Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;
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
	static DTBigramIntFeature *_freeList;
#endif
};

#endif
