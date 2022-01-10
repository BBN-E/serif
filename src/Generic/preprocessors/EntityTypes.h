// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include <iostream>
#include "Generic/common/Symbol.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"

namespace DataPreprocessor {

	class EntityTypes {
	public:
		EntityTypes();
		EntityTypes(const char *file_name, bool useDefault=true);

		Symbol lookup(Symbol foundType) const;

		/// Writes a description of the map to the given output stream.
		void dump(std::ostream &out, int indent = 0) const;

	private:
		void init(const char *file_name, bool useDefault);

		SymbolSubstitutionMap *_typeMap;

	};

} // namespace DataPreprocessor


#endif
