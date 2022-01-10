// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_OPERATOR_EXTRACTOR_H
#define EN_OPERATOR_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

#include <set>

/**
  *  Extracts all propositions associated with a mention.
  */
class EnglishOperatorExtractor : public AttributeValuePairExtractor<Mention> {
public:
	EnglishOperatorExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);

protected:
	std::set<Symbol> _operators;
};

#endif
