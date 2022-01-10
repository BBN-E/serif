// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPOSITION_EXTRACTOR_H
#define PROPOSITION_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Extracts all propositions associated with a mention.
  */
class PropositionExtractor : public AttributeValuePairExtractor<Mention> {
public:
	PropositionExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
