// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/preprocessors/RelationTypeMap.h"
#include <boost/scoped_ptr.hpp>


using namespace std;


RelationTypeMap::RelationTypeMap() {
	init(NULL);
}

RelationTypeMap::RelationTypeMap(const char *file_name) {
	init(file_name);
}

void RelationTypeMap::init(const char *file_name) {
	_typeMap = _new SymbolSubstitutionMap();

	// add the basic relation types
	for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) 
		_typeMap->add(RelationTypeSet::getRelationSymbol(i), RelationTypeSet::getRelationSymbol(i));

	if (file_name != NULL) {
		wstring line;
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(file_name);

		// Read the number of entries.
		int numEntries;
		in >> numEntries;
		in.getLine(line);

		// Read in each of the entires.
		for (int i = 0; i < numEntries; i++) {
			in.getLine(line);
			size_t pos = line.find(L'=');

			if (pos == wstring::npos) {
				Symbol sym = Symbol(line.c_str());
				_typeMap->add(sym, sym);
			}
			else {
				wstring name, replace;
				name = line.substr(0, pos);
				replace = line.substr(pos + 1);
				Symbol name_sym = Symbol(name.c_str());
				Symbol replace_sym = Symbol(replace.c_str());
				if (!_typeMap->contains(replace_sym)) {
					char message[200];
					sprintf(message, "Input type %s for %s does not map to valid relation type.", replace_sym.to_debug_string(), name_sym.to_debug_string());
					throw UnexpectedInputException("RelationTypeMap::init()", message);
				}
				_typeMap->add(name_sym, replace_sym);
			}
		}
		in.close();
	}
}

Symbol RelationTypeMap::lookup(Symbol foundType) const {
	Symbol ret = _typeMap->replace(foundType);
	if (ret == foundType) {
		return (_typeMap->contains(foundType) ? foundType : Symbol(L"NONE"));
	}
	return ret;
}

/**
 * @param out the output stream to write to.
 * @param indent the number of characters to indent.
 */
void RelationTypeMap::dump(std::ostream &out, int indent) const {
	_typeMap->dump(out, indent);
}


