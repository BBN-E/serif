// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TYPE_SET_H
#define RELATION_TYPE_SET_H
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"
#include <string>

// static symbols that represent EDT types
#define MAX_RELATION_TYPES 1000

class RelationTypeSet {
public:
	static int N_RELATION_TYPES;
	static int INVALID_TYPE;

	static void initialize();

	static Symbol getRelationSymbol(int index) {
		if (index == 0) 
			return relationConstants[index];
		if (isNull(index))
			return SymbolConstants::nullSymbol;
		if (index >= 0)
			return relationConstants[index];
		else return reversedRelationConstants[index * -1];
	}

	static Symbol getNormalizedRelationSymbol(int index) {
		if (index == 0) 
			return relationConstants[index];
		if (isNull(index))
			return SymbolConstants::nullSymbol;
		if (index >= 0)
			return relationConstants[index];
		else return relationConstants[index * -1];
	}

	static int reverse(int index) {
		return index * -1;
	}

	static bool isReversed(int index) {
		return (index < 0);
	}

	static bool isNull(int index) {
		return (index == 0 || index == INVALID_TYPE 
			|| index >= N_RELATION_TYPES || index <= -1 * N_RELATION_TYPES);
	}

	static bool isSymmetric(int index) {
		if (isNull(index))
			return false;
		if (index < 0)
			index *= -1;
		return (reversedRelationConstants[index].is_null());
	}

	static int getTypeFromSymbol(Symbol sym) {
		// stupid!
		for (int i = 0; i < N_RELATION_TYPES; i++) {
			if (sym == relationConstants[i])
				return i;
			if (sym == reversedRelationConstants[i])
				return i * -1;
		}
		return INVALID_TYPE;
	}

private:
	static Symbol relationConstants[MAX_RELATION_TYPES];
	static Symbol reversedRelationConstants[MAX_RELATION_TYPES];

	static void read_in_relation_types(const char *filename);

	static bool is_initialized;
	
};



#endif
