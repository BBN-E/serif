// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_ParentheticalExtractor.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>

EnglishParentheticalExtractor::EnglishParentheticalExtractor() : 
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"parenthetical")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishParentheticalExtractor::extractFeatures(const Mention& context,
                                                                           LinkInfoCache& cache,
                                                                           const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	if (context.getEntityType().getName() == Symbol(L"POG")) {
		// Keep the string of any parenthetical mentions for later merging
		std::vector<Symbol> mentionTerminals = context.getNode()->getTerminalSymbols();
		std::vector<std::wstring> mentionTerminalStrings;
		BOOST_FOREACH(Symbol mentionTerminal, mentionTerminals) {
			mentionTerminalStrings.push_back(std::wstring(mentionTerminal.to_string()));
		}
		std::wstring mentionText = boost::algorithm::join(mentionTerminalStrings, L" ");
		results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"mention-text"), Symbol(mentionText), getFullName()));
	}
	return results;
}
