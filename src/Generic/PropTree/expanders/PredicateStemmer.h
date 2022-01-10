// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRED_STEMMER_EXPANDER_H
#define PRED_STEMMER_EXPANDER_H

#include "Generic/PropTree/expanders/PropTreeExpander.h"

class PredicateStemmer : public PropTreeExpander {
public:
	PredicateStemmer(float porter_prob, float wn_stem_prob, bool skip_nouns=false, bool skip_verbs=false) : 
	  PropTreeExpander(), _skip_nouns(skip_nouns), _skip_verbs(skip_verbs),
	  PORTER_STEM_PROB(porter_prob), WN_STEM_PROB(wn_stem_prob) {}
	void expand(const PropNodes& pnodes) const;
private:
	bool _skip_nouns, _skip_verbs;
	float PORTER_STEM_PROB;
	float WN_STEM_PROB;
};




#endif
