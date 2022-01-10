// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PARENTHETICAL_PAIR_EXTRACTOR_H
#define EN_PARENTHETICAL_PAIR_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

class LinkInfoCache;

/**
  *  Extracts link between following parenthetical and its preceding mention.
  */
class EnglishParentheticalPairExtractor : public AttributeValuePairExtractor<MentionPair> {
public: 
	EnglishParentheticalPairExtractor();

	std::vector<AttributeValuePair_ptr> extractFeatures(const MentionPair& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
