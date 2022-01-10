// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_WHQ_LINK_EXTRACTOR_H
#define EN_WHQ_LINK_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  If a wh* or other question word link is present, creates a whq-link feature for a pair of mentions.
  *
  *  Links identified by EnglishPreLinker::getWHQLink().
  */
class EnglishWHQLinkExtractor : public AttributeValuePairExtractor<MentionPair> {
public: 
	EnglishWHQLinkExtractor() ;
	std::vector<AttributeValuePair_ptr> extractFeatures(const MentionPair& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
