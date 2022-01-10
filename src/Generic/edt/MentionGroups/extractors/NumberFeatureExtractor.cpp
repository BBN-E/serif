// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/NumberFeatureExtractor.h"
#include "Generic/edt/Guesser.h"

NumberFeatureExtractor::NumberFeatureExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"number"))
{
	validateRequiredParameters();
}

void NumberFeatureExtractor::validateRequiredParameters() {
	Guesser::initialize();
}

std::vector<AttributeValuePair_ptr> NumberFeatureExtractor::extractFeatures(const Mention& context,
                                                                            LinkInfoCache& cache,
                                                                            const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	Symbol number = Guesser::guessNumber(context.getNode(), &context);
	results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"number"), number, getFullName()));
	return results;
}
