// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Entity.h"

class Constraint {
public:
	int left;
	int right;
	Symbol type;
	EntityType entityType;
	Constraint(int left, int right, Symbol type)
		: left(left), right(right), type(type) {}
	Constraint(int left, int right, Symbol type, EntityType entityType)
		: left(left), right(right), type(type), entityType(entityType) {}
	// If you use the following (default) constructor, then the value of
	// left & right are undefined, so be sure to set them!
	Constraint() {}
};

#endif
