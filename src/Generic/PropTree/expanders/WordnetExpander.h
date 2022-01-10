// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef WN_EXPANDER_H
#define WN_EXPANDER_H

#include "PropTreeExpander.h"
#include "Generic/common/ParamReader.h"

// Applies WordNet synset lookup, creating additional predicates
class WordnetExpander : public PropTreeExpander {
public:
	WordnetExpander(float syn_prob, float hyper_prob, float hypo_prob, float meronym_prob, float similar_prob, float antonym_prob) : 
	  PropTreeExpander(), 
	  WN_SYNONYM_PROB(syn_prob), 
	  WN_HYPONYM_PROB(hypo_prob), WN_HYPERNYM_PROB(hyper_prob), WN_MERONYM_PROB(meronym_prob) ,
	  WN_SIMILAR_PROB(similar_prob), WN_ANTONYM_PROB(antonym_prob)
	{
		_sense_count_maximum = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_expansion_sense_count_maximum", 1000);
	}
	void expand(const PropNodes& pnodes) const;


private:
	float WN_SYNONYM_PROB;  
	float WN_HYPONYM_PROB;  
	float WN_HYPERNYM_PROB; 
	float WN_MERONYM_PROB;
	float WN_ANTONYM_PROB;
	float WN_SIMILAR_PROB;

	int _sense_count_maximum;
	static bool allowExpansion( Symbol p );
};

#endif

