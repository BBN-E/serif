// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRODUCTION_H
#define PRODUCTION_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"

#define MAX_RIGHT_SIDE_SYMBOLS 250

class Production {
public:

    Symbol left;
	Symbol right[MAX_RIGHT_SIDE_SYMBOLS];
	int number_of_right_side_elements;

	Production() : number_of_right_side_elements(0) {}
	

};

#endif

