// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_3INT_FEATURE_H
#define D_T_3INT_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

#ifdef _WIN32
#define swprintf _snwprintf
#endif

using namespace std;

/** DTIntFeature is a feature used to store tag-integer pairs.
  * Where relevant, the tag represents the "future" value of the feature.
  */

class DT3IntFeature : public DTFeature {
public:
	DT3IntFeature(const DTFeatureType *type, Symbol tag, int integer1, 
		int integer2, int integer3)
		: DTFeature(type), _tag(tag), _int1(integer1), _int2(integer2),
		_int3(integer3)
	{}

	void deallocate() { delete this; }

	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DT3IntFeature *otherFeature =
			static_cast<const DT3IntFeature*>(&other);
		return (_tag == otherFeature->_tag &&
				_int1 == otherFeature->_int1 &&				
				_int2 == otherFeature->_int2 &&
				_int3 == otherFeature->_int3);
	}

	size_t getHashCode() const {
		return getFeatureType()->getName().hash_code() ^
			   _tag.hash_code() ^ ((unsigned) _int1 << 3) ^ 
			   ((unsigned) _int2 << 3) ^((unsigned) _int3 << 3) ;
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DT3IntFeature *otherFeature =
			static_cast<const DT3IntFeature*>(&other);
		return (_int1 == otherFeature->_int1 &&				
				_int2 == otherFeature->_int2 &&
				_int3 == otherFeature->_int3);
	}

	size_t getHashCodeWithoutTag() const {
		return getFeatureType()->getName().hash_code() ^
			   ((unsigned) _int1 << 3) ^ 
			   ((unsigned) _int2 << 3) ^((unsigned) _int3 << 3) ;
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	void toString(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d %d", _int1, _int2, _int3);

		str = _tag.to_string();
		str += L" ";
		str += buf;
	}

	void toStringWithoutTag(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d %d", _int1, _int2, _int3);
		str = buf;
	}

	void write(UTF8OutputStream &out) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d %d %d", _int1, _int2, _int3);

		out << _tag.to_string() << L" " << buf;
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_tag = token.symValue();
		in >> _int1;		
		in >> _int2;
		in >> _int3;
	}

	Symbol getTag() const {
		return _tag;
	}

private:
	Symbol _tag;
	int _int1;
	int _int2;
	int _int3;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 10000;
	static DT3IntFeature *_freeList;
#endif
};

#endif
