// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef GPE_AFFILIATION_EXTRACTOR_H
#define GPE_AFFILIATION_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Extracts GPE modifiers and known GPE affiliations for a mention.
  *
  *  GPE modifiers (identified via syntactic and propositional structures) 
  *  must also appear on a list of nationalities controlled by the 
  *  'dt_coref_nationalities' parameter.
  *
  *  GPE affiliations are determined via a lookup table that maps known
  *  names to affiliated GPEs (e.g. "Bush" -> "United States").  The lookup
  *  table is controlled by the 'dt_coref_name_gpe_affiliations' parameter.
  */
class GPEAffiliationExtractor : public AttributeValuePairExtractor<Mention> {
public:
	GPEAffiliationExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
protected:
	void validateRequiredParameters();
	bool _use_awake_info;
};

#endif
