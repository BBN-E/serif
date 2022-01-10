// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef AM_EXPANDER_H
#define AM_EXPANDER_H

#include "PropTreeExpander.h"


class Americanizer : public PropTreeExpander {
public:
	Americanizer() : PropTreeExpander() {}
	void expand(const PropNodes& pnodes) const;
	static std::wstring americanizeWord(const std::wstring& word);

};


#endif

