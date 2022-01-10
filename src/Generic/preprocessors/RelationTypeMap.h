// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TYPE_MAP_H
#define RELATION_TYPE_MAP_H

#include <iostream>
#include "Generic/common/Symbol.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"


class RelationTypeMap {
	public:
		RelationTypeMap();
		RelationTypeMap(const char *file_name);

		Symbol lookup(Symbol foundType) const;

		/// Writes a description of the map to the given output stream.
		void dump(std::ostream &out, int indent = 0) const;

	private:
		void init(const char *file_name);

		SymbolSubstitutionMap *_typeMap;

	};



#endif
