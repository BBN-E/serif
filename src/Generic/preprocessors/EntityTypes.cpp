// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/EntityType.h"
#include "Generic/preprocessors/EntityTypes.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

namespace DataPreprocessor {

EntityTypes::EntityTypes() {
	init(NULL, true);
}

EntityTypes::EntityTypes(const char *file_name, bool useDefault) {
	init(file_name, useDefault);
}

void EntityTypes::init(const char *file_name, bool useDefault) {
	_typeMap = _new SymbolSubstitutionMap();
	if (useDefault) {
		_typeMap->add(Symbol(L"ORGANIZATION"), Symbol(L"ORG"));
		_typeMap->add(Symbol(L"PERSON"), Symbol(L"PER"));
		_typeMap->add(Symbol(L"LOCATION"), Symbol(L"LOC"));
		_typeMap->add(Symbol(L"WEAPON"), Symbol(L"WEA"));
		_typeMap->add(Symbol(L"VEHICLE"), Symbol(L"VEH"));
		_typeMap->add(Symbol(L"GPE"), Symbol(L"GPE"));
		_typeMap->add(Symbol(L"FAC"), Symbol(L"FAC"));
		_typeMap->add(Symbol(L"ORG"), Symbol(L"ORG"));
		_typeMap->add(Symbol(L"PER"), Symbol(L"PER"));
		_typeMap->add(Symbol(L"LOC"), Symbol(L"LOC"));
		_typeMap->add(Symbol(L"WEA"), Symbol(L"WEA"));
		_typeMap->add(Symbol(L"VEH"), Symbol(L"VEH"));
	}
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
				_typeMap->add(Symbol(name.c_str()), Symbol(replace.c_str()));
			}
		}
		in.close();
	}
}

Symbol EntityTypes::lookup(Symbol foundType) const {
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
void EntityTypes::dump(std::ostream &out, int indent) const {
	_typeMap->dump(out, indent);
}

} // namespace DataPreprocessor
