// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_GPENameVariationsExtractor.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

const Symbol EnglishGPENameVariationsExtractor::FULLEST = Symbol(L"fullest");
const Symbol EnglishGPENameVariationsExtractor::SINGLE_WORD = Symbol(L"single-word");

EnglishGPENameVariationsExtractor::EnglishGPENameVariationsExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"gpe-name-variations"))
{
	validateRequiredParameters();
}

/* This method is a direct adaptation of EnglishRuleNameLinker::generateGPEVariations() */
std::vector<AttributeValuePair_ptr> EnglishGPENameVariationsExtractor::extractFeatures(const Mention& context, 
																				 LinkInfoCache& cache,
																				 const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;

	if (context.getEntityType() == EntityType::getGPEType() && 
		(context.getMentionType() == Mention::NAME || context.getMentionType() == Mention::NEST)) 
	{
		std::vector<Symbol> words = context.getHead()->getTerminalSymbols();
		size_t word_count = words.size();

		// concat all but last word
		std::wstring name_str = L"";
		for (size_t i = 0; i < word_count - 1; i++) {
			name_str += words[i].to_string() + std::wstring(L" ");
		}
	
		// check alternate spelling for the last word in a GPE name. 
		// (for example, add "north korea" in place of "north koreans")
		Symbol alt = cache.lookupAlternateSpelling(words.back());
		if (!alt.is_null()) {
			std::wstring alt_str = name_str + alt.to_string();
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FULLEST, alt_str, cache);
		}

		// add final word
		name_str += words.back().to_string(); 

		if (word_count == 1) {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), SINGLE_WORD, name_str, cache);
		} else {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FULLEST, name_str, cache);
		}
	}

	return results;
}
