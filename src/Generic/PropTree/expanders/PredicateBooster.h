// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRED_BOOST_EXPANDER_H
#define PRED_BOOST_EXPANDER_H

#include "PropTreeExpander.h"


class PredicateBooster : public PropTreeExpander {
public:
	PredicateBooster(float mem_prob, float unk_prob, float poss_prob, float of_prob) : 
	  PropTreeExpander(), BOOST_MEMBER_PROB(mem_prob), BOOST_UNKNOWN_PROB(unk_prob),
	  BOOST_POSS_PROB(poss_prob), BOOST_OF_PROB(of_prob) {}
	void expand(const PropNodes& pnodes) const;
private:
	float BOOST_MEMBER_PROB; 
	float BOOST_UNKNOWN_PROB;
	float BOOST_POSS_PROB;   
	float BOOST_OF_PROB;     
};

#endif

