// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_VARIABLE_SIZE_FEATURE_H
#define D_T_VARIABLE_SIZE_FEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"

#include "Generic/discTagger/DTFeature.h"

using namespace std;

const int MAX_DT_FEATURE_SYMBOLS = 10;

/** DTVariableSizeFeature is a feature used to store a variably-sized set
  * of words. Each "word" can come from anywhere, but can't have spaces or 
  * parens in it (because it is written and read as a single UTF8Token). 
  * Where relevant, the first Symbol should represent the "future" value 
  * of the feature.
  */

class DTVariableSizeFeature : public DTFeature {
public:
	DTVariableSizeFeature(const DTFeatureType *type, Symbol s1, const Symbol *symbols, int n_symbols)
		: DTFeature(type), _n_symbols(n_symbols)
	{
		if (_n_symbols > MAX_DT_FEATURE_SYMBOLS) 
			throw UnexpectedInputException("DTVariableSizeFeature::DTVariableSizeFeature()",
										   "Number of symbols in feature exceeds MAX_FEATURE_SYMBOLS");
		
		if (s1.is_null()) 
			throw InternalInconsistencyException(
					"DTVariableSizeFeature::DTVariableSizeFeature()",
					"Attempt to create DTVariableSizeFeature with null Symbol");
		_symbol1 = s1;

		for (int i = 0; i < _n_symbols; i++) {
			if (symbols[i].is_null()) {
				throw InternalInconsistencyException(
					"DTVariableSizeFeature::DTVariableSizeFeature()",
					"Attempt to create DTVariableSizeFeature with null Symbol");
			}
			_symbols[i] = symbols[i];
		}

	}

	void deallocate() { delete this; }


	bool operator==(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTVariableSizeFeature *otherFeature =
			static_cast<const DTVariableSizeFeature*>(&other);
		if (_n_symbols != otherFeature->_n_symbols)
			return false;
		if (_symbol1 != otherFeature->_symbol1)
			return false;
		for (int i = 0; i < _n_symbols; i++) {
			if (!(_symbols[i] == otherFeature->_symbols[i]))
				return false;
		}
		return true;
	}

	size_t getHashCode() const {
		Symbol featureName = getFeatureType()->getName();
		size_t hash_code = featureName.hash_code() ^ _symbol1.hash_code();
		for (int i = 0; i < _n_symbols; i++) 
			hash_code = hash_code ^ _symbols[i].hash_code();
		return hash_code;
	}

	bool equalsWithoutTag(const DTFeature &other) const {
		if (getFeatureType() != other.getFeatureType())
			return false;
		const DTVariableSizeFeature *otherFeature =
			static_cast<const DTVariableSizeFeature*>(&other);
		if (_n_symbols != otherFeature->_n_symbols)
			return false;
		for (int i = 0; i < _n_symbols; i++) {
			if (!(_symbols[i] == otherFeature->_symbols[i]))
				return false;
		}
		return true;
	}

	size_t getHashCodeWithoutTag() const {
		Symbol featureName = getFeatureType()->getName();
		size_t hash_code = featureName.hash_code();
		for (int i = 0; i < _n_symbols; i++) 
			hash_code = hash_code ^ _symbols[i].hash_code();
		return hash_code;
	}

	void toString(wstring &str) const {
		str = _symbol1.to_string();
		str += L" ";
		wchar_t tmp[4];
#if defined(_WIN32)
		str += _itow(_n_symbols, tmp, 3);
#else
		swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d",_n_symbols);
		str += tmp;
#endif
		for (int i = 0; i < _n_symbols; i++) {
			str += L" ";
			str += _symbols[i].to_string();
		}
	}

	void toStringWithoutTag(wstring &str) const {
		wchar_t tmp[4];
#if defined(_WIN32)
		str = _itow(_n_symbols, tmp, 3);
#else
		swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d",_n_symbols);
		str = tmp;
#endif
		for (int i = 0; i < _n_symbols; i++) {
			str += L" ";
			str += _symbols[i].to_string();
		}
	}


	void write(UTF8OutputStream &out) const {
		out << _symbol1.to_string();
		out << L" ";
		out << _n_symbols;
		for (int i = 0; i < _n_symbols; i++) {
			out << L" ";
			out << _symbols[i].to_string();
		}
	}

	void read(UTF8InputStream &in) {
		UTF8Token token;

		in >> token;
		_symbol1 = token.symValue();

		in >> token;
		_n_symbols = _wtoi(token.chars());

		if (_n_symbols > MAX_DT_FEATURE_SYMBOLS) 
			throw UnexpectedInputException("DTVariableSizeFeature::read()",
										   "Number of symbols in feature exceeds MAX_DT_FEATURE_SYMBOLS");

		for (int i = 0; i < _n_symbols; i++) {
			in >> token;
			_symbols[i] = token.symValue();
		}
	}

	Symbol getTag() const {
		return _symbol1;
	}


private:
	Symbol _symbol1;
	Symbol _symbols[MAX_DT_FEATURE_SYMBOLS];
	int _n_symbols;

#ifdef ALLOCATION_POOLING
	// memory pooling stuff
public:
	static void* operator new(size_t n, int, char *, int)
		{ return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void *object);
private:
	static const size_t _block_size = 100000;
	static DTVariableSizeFeature *_freeList;
#endif
};

#endif
