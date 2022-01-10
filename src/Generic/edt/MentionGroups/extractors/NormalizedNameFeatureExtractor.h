// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NORMALIZED_NAME_FEATURE_EXTRACTOR_H
#define NORMALIZED_NAME_FEATURE_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Generates a single, normalized name string for a mention.
  */
class NormalizedNameFeatureExtractor : public AttributeValuePairExtractor<Mention> {
public:
	NormalizedNameFeatureExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
