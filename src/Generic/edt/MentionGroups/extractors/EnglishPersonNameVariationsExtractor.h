// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENGLISH_PERSON_NAME_VARIATIONS_EXTRACTOR_H
#define ENGLISH_PERSON_NAME_VARIATIONS_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/common/SymbolHash.h"

#include <boost/scoped_ptr.hpp>

class Mention;

/**
  *  Extracts variants for PER names.
  *
  * This is included in the Generic module, rather than the English
  * module, because non-English documents often use English naming
  * conventions when mentioning names of English-speaking people.
  *
  *  Adapted from EnglishRuleNameLinker::generatePERVariations().  
  */
class EnglishPersonNameVariationsExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	EnglishPersonNameVariationsExtractor(); 

	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

	static const Symbol FULLEST;
	static const Symbol FIRST_MI_LAST;
	static const Symbol FIRST_LAST;
	static const Symbol FIRST;
	static const Symbol LAST;
	static const Symbol MI;
	static const Symbol MIDDLE;
	static const Symbol MIDDLE_TOKEN;
	static const Symbol SUFFIX;

protected:
	void validateRequiredParameters();

private:
	boost::scoped_ptr<SymbolHash> _suffixes;
};

#endif
