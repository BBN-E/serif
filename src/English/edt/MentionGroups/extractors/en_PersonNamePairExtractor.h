// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PERSON_NAME_PAIR_EXTRACTOR_H
#define EN_PERSON_NAME_PAIR_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

#include <vector>

class LinkInfoCache;

/**
  *  Extracts first, last and suffix name match and clash features for pairs of PER names.
  */
class EnglishPersonNamePairExtractor : public AttributeValuePairExtractor<MentionPair> {
public: 
	EnglishPersonNamePairExtractor();

	std::vector<AttributeValuePair_ptr> extractFeatures(const MentionPair& context, LinkInfoCache& cache, const DocTheory *docTheory);

private:

	void addMatchAndClashFeatures(std::vector<AttributeValuePair_ptr>& results,
	                              LinkInfoCache& cache,
	                              const Mention *m1, const Mention *m2,
	                              Symbol extractorName, Symbol featureName,
								  std::vector<Symbol> &allNameFeatures);
};



#endif
