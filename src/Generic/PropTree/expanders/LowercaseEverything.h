// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOWER_ALL_EXPANDER_H
#define LOWER_ALL_EXPANDER_H

#include "PropTreeExpander.h"

class LowercaseEverything : public PropTreeExpander {
public:
	LowercaseEverything() : PropTreeExpander() {}
	void expand(const PropNodes& pnodes) const;
};


#endif

