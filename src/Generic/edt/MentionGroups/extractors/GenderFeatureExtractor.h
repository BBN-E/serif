// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENDER_FEATURE_EXTRACTOR_H
#define GENDER_FEATURE_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"
#include <set>

/**
  *  Uses the Guesser to extract a likely grammatical gender for a mention.
  */
class GenderFeatureExtractor : public AttributeValuePairExtractor<Mention> {
public:
	GenderFeatureExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
	void resetForNewDocument(const DocTheory *docTheory);
protected:
	void validateRequiredParameters();
private:
	std::set<Symbol> _suspectedSurnames;
};

#endif
