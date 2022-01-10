// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SHARED_PROPOSITION_EXTRACTOR_H
#define SHARED_PROPOSITION_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Extracts all propositions associated with both mentions in a MentionPair.
  */
class SharedPropositionExtractor : public AttributeValuePairExtractor<MentionPair> {
public:
	SharedPropositionExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const MentionPair& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
