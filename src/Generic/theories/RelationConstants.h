// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_CONSTANTS_H
#define RELATION_CONSTANTS_H
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <string>
#include <map>

class RelationConstants {

private:
	// Cache the base & subtype symbols so we don't need to keep
	// recomputing them in inner loops (such as feature functions)
	typedef Symbol::HashMap<Symbol> SymSymMap;
	static SymSymMap _baseTypeSymbol;
	static SymSymMap _subtypeSymbol;

public:

	static Symbol NONE;	
	static Symbol RAW;
	static Symbol IDENTITY;

	// not actually in the set unless you define it! But here so that
	// generics can test against it
	static Symbol ROLE_CITIZEN_OF;

	static Symbol getBaseTypeSymbol(const Symbol &t) {
	    Symbol& result = _baseTypeSymbol[t];
	    if (result.is_null()) {
			std::wstring str = t.to_string();
			size_t index = str.find(L".");
			result = Symbol(str.substr(0, index).c_str());
	    }
	    return result;
	}

	static Symbol getSubtypeSymbol(const Symbol &t) {
	    Symbol& result = _subtypeSymbol[t];
	    if (result.is_null()) {
			std::wstring str = t.to_string();
			size_t index = str.find(L".");
			if (index >= str.length())
				result = NONE;
			// e.g. "DISC."
			else if (index == str.length() - 1)
				result = NONE;
			else 
				result = Symbol(str.substr(index + 1).c_str());
	    }
	    return result;
	}
	
};



#endif
