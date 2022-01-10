// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_SET_H
#define SYMBOL_SET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"

class SymbolSet {
public:
	SymbolSet() : _symbols(0), _n_symbols(0), _hash_value(0) {}

	SymbolSet(int n_symbols, Symbol *symbols) : _symbols(0) {
		if (n_symbols > 0)
			_symbols = _new Symbol[n_symbols];

		// sort symbols 
		_n_symbols = 0;
		for (int i = 0; i < n_symbols; i++) {
			if (symbols[i].is_null())
				throw InternalInconsistencyException("SymbolSet::SymbolSet()", "NULL symbol added to SymbolSet");
			bool inserted = false;
			for (int j = 0; j < _n_symbols; j++) {
				// ignore any duplicates
				if (_symbols[i] == _symbols[j]) {
					inserted = true;
					break;
				}
				if (wcscmp(symbols[i].to_string(), _symbols[j].to_string()) < 0) {
					for (int k = _n_symbols; k > j; k--) 
						_symbols[k] = _symbols[k-1];
					_symbols[j] = symbols[i];
					_n_symbols++;
					inserted = true;
					break;
				}
			}
			if (!inserted) 
				_symbols[_n_symbols++] = symbols[i];
		}

		// store hash code for later use
		_hash_value = hash_code();

	}

	SymbolSet(GrowableArray<Symbol> &symArray) : _symbols(0) {
		if (symArray.length() > 0)
			_symbols = _new Symbol[symArray.length()];

		// sort symbols 
		_n_symbols = 0;
		for (int i = 0; i < symArray.length(); i++) {
			if (symArray[i].is_null())
				throw InternalInconsistencyException("SymbolSet::SymbolSet()", "NULL symbol added to SymbolSet");
			bool inserted = false;
			for (int j = 0; j < _n_symbols; j++) {
				// ignore any duplicates
				if (symArray[i] == _symbols[j]) {
					inserted = true;
					break;
				}
				if (wcscmp(symArray[i].to_string(), _symbols[j].to_string()) < 0) {
					for (int k = _n_symbols; k > j; k--) 
						_symbols[k] = _symbols[k-1];
					_symbols[j] = symArray[i];
					_n_symbols++;
					inserted = true;
					break;
				}
			}
			if (!inserted) 
				_symbols[_n_symbols++] = symArray[i];
		}

		// store hash code for later use
		_hash_value = hash_code();
	}

	// copy constructor
	SymbolSet(const SymbolSet &sg) : _symbols(0) {
		_n_symbols = sg._n_symbols;
		if (_n_symbols > 0)
			_symbols = _new Symbol[_n_symbols];
		for (int i = 0; i < _n_symbols; i++) 
			_symbols[i] = sg._symbols[i];
	
		_hash_value = sg._hash_value;
	}

	~SymbolSet() { if (_n_symbols > 0) { delete [] _symbols; } }

	size_t hash_value() const { return _hash_value; }

	bool operator== (const SymbolSet &sg2) const {
		if (_n_symbols != sg2._n_symbols)
			return false;

		for (int i = 0; i < _n_symbols; i++) {
			if (_symbols[i] != sg2._symbols[i])
				return false;
		}

		return true;
	}

	bool operator!= (const SymbolSet &sg2) const {
		return !((*this) == sg2);
	}

	SymbolSet& operator=(const SymbolSet &sg) {
		if (&sg != this) {
			if (_n_symbols != sg._n_symbols) {
				if  (_n_symbols > 0)
					delete [] _symbols;
				_n_symbols = sg._n_symbols;
				if (_n_symbols > 0)
					_symbols = _new Symbol[_n_symbols];
			}
			for (int i = 0; i < _n_symbols; i++) 
				_symbols[i] = sg._symbols[i];
	
			_hash_value = sg._hash_value;
		}

		return *this;
	}

	Symbol operator[](int index) const {
		if (index >= 0 && index < _n_symbols)
			return _symbols[index];
		else
			throw InternalInconsistencyException("SymbolSet::operator[]()", "Array index out of bounds");
	}

	int getNSymbols() const {
		return _n_symbols;
	}

	Symbol getSymbol(int index) const {
		if (index >= 0 && index < _n_symbols)
			return _symbols[index];
		else
			throw InternalInconsistencyException("SymbolSet::getSymbol()", "Array index out of bounds");
	}

	std::wstring to_string() const {
		std::wstring result = L"(";
		for (int i = 0; i < _n_symbols - 1; i++) {
			result += _symbols[i].to_string();
			result += L' ';
		}
		if (_n_symbols > 0)
			result += _symbols[_n_symbols - 1].to_string();
		result += L")";
		return result;
	}

private:
	Symbol *_symbols;
	int _n_symbols;

	size_t _hash_value;

	size_t hash_code() const {
		size_t val = 0;
        for (int i = 0; i < _n_symbols; i++)
            val = (val << 2) + _symbols[i].hash_code();
        return val;
	}

public:
	friend UTF8InputStream& operator>>(UTF8InputStream& stream, SymbolSet& sg)
		throw(UnexpectedInputException)
	{
		UTF8Token token;
		GrowableArray<Symbol> symArray;

		stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) 
		  throw UnexpectedInputException("SymbolSet::operator>>()", "ERROR: ill-formed SymbolSet.");
        

		stream >> token;

		while (token.symValue() != SymbolConstants::rightParen) {
            symArray.add(token.symValue());
			stream >> token;
        }
		
		if (symArray.length() != sg._n_symbols) {
			if (sg._n_symbols > 0)
				delete [] sg._symbols;
			if (symArray.length() > 0)
				sg._symbols = _new Symbol[symArray.length()];
		}

		// sort symbols 
		sg._n_symbols = 0;
		for (int i = 0; i < symArray.length(); i++) {
			bool inserted = false;
			for (int j = 0; j < sg._n_symbols; j++) {
				// ignore any duplicates
				if (symArray[i] == sg._symbols[j]) {
					inserted = true;
					break;
				}
				if (wcscmp(symArray[i].to_string(), sg._symbols[j].to_string()) < 0) {
					for (int k = sg._n_symbols; k > j; k--) 
						sg._symbols[k] = sg._symbols[k-1];
					sg._symbols[j] = symArray[i];
					sg._n_symbols++;
					inserted = true;
					break;
				}
			}
			if (!inserted)
				sg._symbols[sg._n_symbols++] = symArray[i];
		}

		// store hash code for later use
		sg._hash_value = sg.hash_code();

		return stream;

	}

	friend UTF8OutputStream& operator<<(UTF8OutputStream& stream, const SymbolSet& sg) {
		stream << "(";
		for (int i = 0; i < sg._n_symbols - 1; i++) {
			stream << sg._symbols[i].to_string();
			stream << " ";
		}
		if (sg._n_symbols > 0)
			stream << sg._symbols[sg._n_symbols - 1].to_string();
		stream << ")";
		return stream;
	}
};

#endif
