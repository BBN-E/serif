// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef AGG_EXPANDER_H
#define AGG_EXPANDER_H

#include "MentionExpander.h"

class AggressiveMentionExpander : public MentionExpander {
public:
	AggressiveMentionExpander(bool lowercase) : MentionExpander(lowercase) {}
	void expand(const PropNodes& pnodes) const;
};


#endif

