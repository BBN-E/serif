// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_TITLE_EXTRACTOR_H
#define EN_TITLE_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"

class Mention;

/**
  *  Extracts all title mentions (as identified by EnglishPreLinker::getTitle()).
  */
class EnglishTitleExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishTitleExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
};

#endif
