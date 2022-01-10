// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PARENTHETICAL_EXTRACTOR_H
#define EN_PARENTHETICAL_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"

class Mention;

/**
  *  Extracts all mentions that are inside a parenthetical or reference one,
  *  using the special Mention subtypes.
  */
class EnglishParentheticalExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishParentheticalExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
