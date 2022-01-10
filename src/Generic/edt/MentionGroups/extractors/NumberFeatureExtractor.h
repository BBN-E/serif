// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUMBER_FEATURE_EXTRACTOR_H
#define NUMBER_FEATURE_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Uses the Guesser to extract a likely grammatical number for a mention.
  */
class NumberFeatureExtractor : public AttributeValuePairExtractor<Mention> {
public:
	NumberFeatureExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
protected:
	void validateRequiredParameters();
};

#endif
