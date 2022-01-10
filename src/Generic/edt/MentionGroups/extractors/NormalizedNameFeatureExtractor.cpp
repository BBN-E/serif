// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/NormalizedNameFeatureExtractor.h"
#include "Generic/theories/SynNode.h"

NormalizedNameFeatureExtractor::NormalizedNameFeatureExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"normalized-name")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> NormalizedNameFeatureExtractor::extractFeatures(const Mention& context,
                                                                                    LinkInfoCache& cache,
                                                                                    const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	if (context.getMentionType() == Mention::NAME || context.getMentionType() == Mention::NEST) {
		std::vector<Symbol> terminalSymbols = context.getHead()->getTerminalSymbols();
		std::vector<Symbol>::const_iterator it = terminalSymbols.begin();
		if (it != terminalSymbols.end()) {
			std::wstring name = (*it).to_string();
			for ( ++it; it != terminalSymbols.end(); ++it) {
				name += L" ";
				name += (*it).to_string();
			}
			results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"name"), Symbol(name), getFullName()));
		}
	}
	return results;
}
