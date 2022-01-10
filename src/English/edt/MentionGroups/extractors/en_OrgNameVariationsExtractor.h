// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ORG_NAME_VARIATIONS_EXTRACTOR_H
#define EN_ORG_NAME_VARIATIONS_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/common/SymbolHash.h"

#include <boost/scoped_ptr.hpp>

class Mention;

/**
  *  Extracts variants for ORG names.
  *
  *  Adapted from EnglishRuleNameLinker::generateORGVariations().  
  */
class EnglishOrgNameVariationsExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishOrgNameVariationsExtractor(); 

	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

protected:
	void validateRequiredParameters();

private:
	boost::scoped_ptr<SymbolHash> _designators;

	static const Symbol NO_DESIG;
	static const Symbol SINGLE_WORD;
	static const Symbol FULLEST;
	static const Symbol MILITARY;
};

#endif
