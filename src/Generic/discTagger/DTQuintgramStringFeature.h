// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_QUINTGRAM_STRING_FEATURE_H
#define D_T_QUINTGRAM_STRING_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeature.h"

#include <string>

using namespace std;


/** DTWordFeature is a feature used to store an n-gram of words. Each "word" can
  * come from anywhere, but can't have spaces or parens in it (because it 
  * is written and read as a single UTF8Token). Where relevant, the first
  * Symbol should represent the "future" value of the feature.

  * DTQuintgramStringFeature is a spin off of DT6GramFeature. It was created because
  * some of the words in DT6GramFeature really should not be put in the Symbol table
  * since they are lists of strings and tags. Note: the wstring word is in the middle 
  * of the list of words due to that word being in the middle of the model file.
  */

class DTQuintgramStringFeature : public DTFeature {
public:
	DTQuintgramStringFeature(const DTFeatureType *type, const Symbol& s1, const Symbol& s2, const Symbol& s3,
		const wstring &str1, const Symbol& s4, const Symbol& s5)
		: DTFeature(type), _symbol1(s1), _symbol2(s2), _symbol3(s3), 
		_string1(str1), _symbol4(s4),  _symbol5(s5)
	{
		if (s1.is_null() || s2.is_null() || 
			s3.is_null() || s4.is_null() || 
			s5.is_null()) {
			throw InternalInconsistencyException(
				"DTQuintgramStringFeature::DTQuintgramStringFeature()",
				"Attempt to create DTQuintgramStringFeature with null Symbol");
		}
	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTQuintgramStringFeature *otherFeature =
			static_cast<const DTQuintgramStringFeature*>(&other);
		return (_symbol1 == otherFeature->_symbol1 &&
				_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_string1 == otherFeature->_string1);
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^
			   _symbol1.hash_code() ^ _symbol2.hash_code() ^ _symbol3.hash_code()
			    ^ _symbol4.hash_code() ^ _symbol5.hash_code() ^ Symbol::hash_str(_string1.c_str());
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTQuintgramStringFeature *otherFeature =
			static_cast<const DTQuintgramStringFeature*>(&other);
		return (_symbol2 == otherFeature->_symbol2 &&
				_symbol3 == otherFeature->_symbol3 &&
				_symbol4 == otherFeature->_symbol4 &&
				_symbol5 == otherFeature->_symbol5 &&
				_string1 == otherFeature->_string1);
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		return featureName.hash_code() ^ _symbol2.hash_code() ^ _symbol3.hash_code()
			^ _symbol4.hash_code() ^ _symbol5.hash_code() ^ Symbol::hash_str(_string1.c_str());
	}


	void toString(wstring &str) const {
		str = _symbol1.to_string();
		str += L" ";
		str += _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _string1;
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += _symbol5.to_string();
	}

	void toStringWithoutTag(wstring &str) const {
		str += _symbol2.to_string();
		str += L" ";
		str += _symbol3.to_string();
		str += L" ";
		str += _string1;
		str += L" ";
		str += _symbol4.to_string();
		str += L" ";
		str += _symbol5.to_string();
	}

	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string() << L" " << _symbol2.to_string() << L" ";
		out << _symbol3.to_string() << L" " << _string1 << L" ";
		out << _symbol4.to_string() << L" " <<_symbol5.to_string();
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
		_string1 = token.chars();
		in >> token;
		_symbol4 = token.symValue();
		in >> token;
		_symbol5 = token.symValue();
	}

	Symbol getTag() const {
		return _symbol1;
	}

private:
	Symbol _symbol1;
	Symbol _symbol2;
	Symbol _symbol3;
	Symbol _symbol4;
	Symbol _symbol5;
	
	wstring _string1;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTQuintgramStringFeature *_freeList;
#endif
};

#endif
