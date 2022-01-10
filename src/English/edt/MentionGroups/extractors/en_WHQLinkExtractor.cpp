// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/extractors/en_WHQLinkExtractor.h"
#include "English/edt/en_PreLinker.h"

EnglishWHQLinkExtractor::EnglishWHQLinkExtractor() :
	AttributeValuePairExtractor<MentionPair>(Symbol(L"MentionPair"), Symbol(L"whq-link")) 
{
	validateRequiredParameters();
}

std::vector<AttributeValuePair_ptr> EnglishWHQLinkExtractor::extractFeatures(const MentionPair& context,
                                                                             LinkInfoCache& cache,
                                                                             const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	const Mention* link1 = EnglishPreLinker::getWHQLink(context.first, context.first->getMentionSet());
	if (link1 != NULL && link1 == context.second) {
		results.push_back(AttributeValuePair<bool>::create(Symbol(L"whq-link"), true, getFullName()));
		return results;
	}
	const Mention* link2 = EnglishPreLinker::getWHQLink(context.second, context.second->getMentionSet());
	if (link2 != NULL && link2 == context.first) {
		results.push_back(AttributeValuePair<bool>::create(Symbol(L"whq-link"), true, getFullName()));
		return results;
	}
	return results;
}
