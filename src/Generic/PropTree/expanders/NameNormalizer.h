// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_NORMALIZER_H
#define NAME_NORMALIZER_H

#include "PropTreeExpander.h"

class NameNormalizer : public PropTreeExpander {
public:
	NameNormalizer() : PropTreeExpander() {}
	void expand(const PropNodes& pnodes) const;
};



#endif

