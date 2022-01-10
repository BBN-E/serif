// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACRONYM_EXTRACTOR_H
#define ACRONYM_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Generates all possible acronyms for a name mention (non-PER, non-GPE).
  */
class AcronymExtractor : public AttributeValuePairExtractor<Mention> {
public:
	AcronymExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
