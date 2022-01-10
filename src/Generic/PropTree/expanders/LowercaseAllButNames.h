// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOWER_EXCEPT_NAMES_EXPANDER_H
#define LOWER_EXCEPT_NAMES_EXPANDER_H

#include "PropTreeExpander.h"


class LowercaseAllButNames : public PropTreeExpander {
public:
	LowercaseAllButNames() : PropTreeExpander() {}
	void expand(const PropNodes& pnodes) const;
};



#endif

