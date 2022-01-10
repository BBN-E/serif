// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/AcronymExtractor.h"

#include "Generic/edt/AcronymMaker.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/theories/SynNode.h"

#include <boost/foreach.hpp>

AcronymExtractor::AcronymExtractor() : 
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"acronym")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> AcronymExtractor::extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory) {
	std::vector<AttributeValuePair_ptr> results;

	if ((context.getMentionType() == Mention::NAME || context.getMentionType() == Mention::NEST) && 
		context.getEntityType() != EntityType::getPERType() && context.getEntityType() != EntityType::getGPEType()) 
	{
		std::vector<Symbol> terminals = context.getHead()->getTerminalSymbols();
		std::vector<Symbol> acronyms = AcronymMaker::getSingleton().generateAcronyms(terminals);
		BOOST_FOREACH(Symbol acronym, acronyms) {
			results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"acronym"), acronym, getFullName()));
		}
	}

	return results;
}
