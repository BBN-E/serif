// Copyright (c) 2016 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/HeadWordExtractor.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SessionLogger.h"

HeadWordExtractor::HeadWordExtractor() : 
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"head-word")) 
{
	validateRequiredParameters();
}

void HeadWordExtractor::validateRequiredParameters() { }

std::vector<AttributeValuePair_ptr> HeadWordExtractor::extractFeatures(const Mention& context,
                                                                       LinkInfoCache& cache,
                                                                       const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	if (context.getMentionType() == Mention::DESC || context.getMentionType() == Mention::PART)
		results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"head-word"), context.getNode()->getHeadWord(), getFullName()));
	return results;
}
