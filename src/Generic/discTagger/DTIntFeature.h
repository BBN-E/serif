// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_INT_FEATURE_H
#define D_T_INT_FEATURE_H

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

class DTIntFeature : public DTFeature {
public:
	DTIntFeature(const DTFeatureType *type, Symbol tag, int integer)
		: DTFeature(type), _tag(tag), _integer(integer)
	{}

	void deallocate() { delete this; }

	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTIntFeature *otherFeature =
			static_cast<const DTIntFeature*>(&other);
		return (_tag == otherFeature->_tag &&
				_integer == otherFeature->_integer);
	}

	size_t getHashCode() const {
		return getFeatureType()->getName().hash_code() ^
			   _tag.hash_code() ^ ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTIntFeature *otherFeature =
			static_cast<const DTIntFeature*>(&other);
		return (_integer == otherFeature->_integer);
	}

	size_t getHashCodeWithoutTag() const {
		return getFeatureType()->getName().hash_code() ^
			   ((unsigned) _integer << 3);
		// taking advantage of assumption that _integer < 2^28 (and
		// non-negative)
	}

	void toString(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d", _integer);

		str = _tag.to_string();
		str += L" ";
		str += buf;
	}

	void toStringWithoutTag(wstring &str) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d", _integer);
		str = buf;
	}

	void write(UTF8OutputStream &out) const {
		wchar_t buf[100];
		swprintf(buf, 100, L"%d", _integer);

		out << _tag.to_string() << L" " << buf;
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;
		in >> token;
		_tag = token.symValue();
		in >> _integer;
	}

	Symbol getTag() const {
		return _tag;
	}

private:
	Symbol _tag;
	int _integer;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 10000;
	static DTIntFeature *_freeList;
#endif
};

#endif
