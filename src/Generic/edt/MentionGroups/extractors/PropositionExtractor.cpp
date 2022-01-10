// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/PropositionExtractor.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/SentenceTheory.h"

PropositionExtractor::PropositionExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"proposition"))
{
	validateRequiredParameters();
}

namespace {
	Symbol PROPOSITION_SYM(L"proposition");
}

std::vector<AttributeValuePair_ptr> PropositionExtractor::extractFeatures(const Mention& context,
                                                                          LinkInfoCache& cache,
                                                                          const DocTheory *docTheory)
{
	int sentno = context.getSentenceNumber();

	std::vector<AttributeValuePair_ptr> results;
	PropositionSet *propSet = docTheory->getSentenceTheory(sentno)->getPropositionSet();
	MentionSet *mentionSet = docTheory->getSentenceTheory(sentno)->getMentionSet();
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		Proposition *prop = propSet->getProposition(p);
		for (int a = 0; a < prop->getNArgs(); a++) {
			if (prop->getArg(a)->getType() == Argument::MENTION_ARG) {
				if (prop->getArg(a)->getMention(mentionSet)->getUID() == context.getUID()) {
					results.push_back(AttributeValuePair<const Proposition*>::create(PROPOSITION_SYM, prop, getFullName()));
					break;						
				}
			}
		}
	}
	return results;
}
