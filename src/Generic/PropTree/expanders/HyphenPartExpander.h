// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef HYPHEN_EXPANDER_H
#define HYPHEN_EXPANDER_H

#include "PropTreeExpander.h"

class HyphenPartExpander : public PropTreeExpander {
public:
	HyphenPartExpander() : PropTreeExpander() {}	  
	void expand(const PropNodes& pnodes) const;
};


#endif

