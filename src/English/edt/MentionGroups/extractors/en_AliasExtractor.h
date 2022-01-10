// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ALIAS_EXTRACTOR_H
#define EN_ALIAS_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"

class Mention;

/**
  *  Extracts aliases mentioned in context (e.g. "Abu Mazen (Mahmoud Abbas)").
  *
  *  Leverages EnglishPreLinker::preLinkContextLinks().
  */
class EnglishAliasExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishAliasExtractor(); 
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 
};

#endif
