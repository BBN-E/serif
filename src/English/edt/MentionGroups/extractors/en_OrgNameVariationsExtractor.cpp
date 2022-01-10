// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_OrgNameVariationsExtractor.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include "English/common/en_WordConstants.h"

const Symbol EnglishOrgNameVariationsExtractor::FULLEST = Symbol(L"fullest");
const Symbol EnglishOrgNameVariationsExtractor::SINGLE_WORD = Symbol(L"single-word");
const Symbol EnglishOrgNameVariationsExtractor::NO_DESIG = Symbol(L"no-designator");
const Symbol EnglishOrgNameVariationsExtractor::MILITARY = Symbol(L"military-unit");

EnglishOrgNameVariationsExtractor::EnglishOrgNameVariationsExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"org-name-variations")),
	_designators(_new SymbolHash(ParamReader::getRequiredParam("linker_designators").c_str()))
{
	validateRequiredParameters();
}

void EnglishOrgNameVariationsExtractor::validateRequiredParameters() {
	ParamReader::getRequiredParam("linker_designators");
}

/* This method is a direct adaptation of EnglishRuleNameLinker::generateORGVariations() */
std::vector<AttributeValuePair_ptr> EnglishOrgNameVariationsExtractor::extractFeatures(const Mention& context,
																				 LinkInfoCache& cache,
																				 const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;

	if (context.getEntityType() == EntityType::getORGType() && 
		(context.getMentionType() == Mention::NAME || context.getMentionType() == Mention::NEST)) 
	{
		std::vector<Symbol> words = context.getHead()->getTerminalSymbols();
		size_t word_count = words.size();

		// Construct the longest name and the longest name without any designators ("inc.", "ltd.", etc.)
		std::wstring fullest_str = L"";
		std::wstring no_desig_str = L"";
		for (std::vector<Symbol>::reverse_iterator it = words.rbegin(); it != words.rend(); ++it) {
			if (!fullest_str.empty()) {
				fullest_str.insert(0, L" ");
			}
			fullest_str.insert(0, (*it).to_string());
			
			if (_designators->lookup(Symbol(fullest_str.c_str()))) {
				no_desig_str = L"";
			} else {
				if (!no_desig_str.empty()) {
					no_desig_str.insert(0, L" ");
				}
				no_desig_str.insert(0, (*it).to_string());
			}
			
		}
		MentionGroupUtils::addSymbolFeatures(results, getFullName(), NO_DESIG, no_desig_str, cache);
		if (word_count == 1) {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), SINGLE_WORD, fullest_str, cache);
		} else {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FULLEST, fullest_str, cache);
		}

		// Military unit variations (e.g.  "The 1st Battalion" --> "The 1st", "1 Battalion")
		if (word_count >= 2) {
			Symbol first = words[0];
			Symbol second = words[1];
			Symbol last = words.back();

			if (WordConstants::isMilitaryWord(last) &&
				(WordConstants::isOrdinal(first) || 
				(first == EnglishWordConstants::THE && WordConstants::isOrdinal(second))))
			{
				Symbol ordinal;
				if (WordConstants::isOrdinal(first)) ordinal = first;
				if (WordConstants::isOrdinal(second)) ordinal = second;
				
				Symbol numberSym = WordConstants::getNumericPortion(ordinal);
				std::wstring the_ordinal = std::wstring(L"the ") + ordinal.to_string();
				std::wstring the_number_last = numberSym.to_string() + std::wstring(L" ") + last.to_string();
				
				MentionGroupUtils::addSymbolFeatures(results, getFullName(), MILITARY, ordinal.to_string(), cache);
				MentionGroupUtils::addSymbolFeatures(results, getFullName(), MILITARY, the_ordinal, cache);
				MentionGroupUtils::addSymbolFeatures(results, getFullName(), MILITARY, the_number_last, cache);
			}	
		}
	}
	return results;
}
