// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_GPE_NAME_VARIATIONS_EXTRACTOR_H
#define EN_GPE_NAME_VARIATIONS_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"

class Mention;

/**
  *  Extracts variants for GPE names from a lookup table.
  *
  *  Adapted from EnglishRuleNameLinker::generateGPEVariations().  Lookup
  *  table is populated by 'linker_alt_spellings' and 'linker_nations'
  *  parameters.
  */
class EnglishGPENameVariationsExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishGPENameVariationsExtractor(); 
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

private:
	static const Symbol SINGLE_WORD;
	static const Symbol FULLEST;
};

#endif
