// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_AliasExtractor.h"
#include "English/edt/en_PreLinker.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"

#include <boost/foreach.hpp>

EnglishAliasExtractor::EnglishAliasExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"alias"))
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishAliasExtractor::extractFeatures(const Mention& context,
																	 LinkInfoCache& cache,
																	 const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;
	PreLinker::MentionMap links;

	const MentionSet *mentionSet = context.getMentionSet();
	const SynNode *node = context.getNode();
	while (!node->isPreterminal()) {
		if (EnglishPreLinker::preLinkContextLinksGivenNode(links, mentionSet, mentionSet->getMention(context.getUID()), node)) {
			BOOST_FOREACH(PreLinker::MentionMap::value_type pair, links) {
				const Mention * aliasMention = (pair.first == context.getIndex()) ? mentionSet->getMention(pair.second->getUID()) : mentionSet->getMention(pair.first);
				results.push_back(AttributeValuePair<const Mention*>::create(Symbol(L"alias"), aliasMention, getFullName()));
			}
			break;
		}
		node = node->getHead();
	}
	
	return results;
}
