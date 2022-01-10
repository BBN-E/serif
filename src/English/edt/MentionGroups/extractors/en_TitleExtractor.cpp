// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_TitleExtractor.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "English/edt/en_PreLinker.h"

#include <boost/foreach.hpp>

EnglishTitleExtractor::EnglishTitleExtractor() : 
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"title")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishTitleExtractor::extractFeatures(const Mention& context,
                                                                           LinkInfoCache& cache,
                                                                           const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	PreLinker::MentionMap links;
	const MentionSet *mentionSet = context.getMentionSet();
	if (EnglishPreLinker::getTitle(mentionSet, &context, &links)) {
		BOOST_FOREACH(PreLinker::MentionMap::value_type pair, links) {
			const Mention * titleMention = mentionSet->getMention(pair.first);
			results.push_back(AttributeValuePair<const Mention*>::create(Symbol(L"title"), titleMention, getFullName()));
		}
	}
	return results;
}
