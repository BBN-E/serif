// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef CONMEN_EXPANDER_H
#define CONMEN_EXPANDER_H

#include "MentionExpander.h"


class ConservativeMentionExpander : public MentionExpander {
public:
	ConservativeMentionExpander(bool lowercase) : MentionExpander(lowercase) {}
	void expand(const PropNodes& pnodes) const;
};



#endif

