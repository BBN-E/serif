// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/SharedPropositionExtractor.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/SentenceTheory.h"

SharedPropositionExtractor::SharedPropositionExtractor() :
	AttributeValuePairExtractor<MentionPair>(Symbol(L"MentionPair"), Symbol(L"shared-proposition"))
{
	validateRequiredParameters();
}

namespace {
	Symbol SHARED_PROPOSITION_SYM(L"shared-proposition");
}

std::vector<AttributeValuePair_ptr> SharedPropositionExtractor::extractFeatures(const MentionPair& context,
                                                                                LinkInfoCache& cache,
                                                                                const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;
	int m1_sentno = context.first->getSentenceNumber();
	int m2_sentno = context.second->getSentenceNumber();

	// A proposition can only link two mentions if they occur in the same sentence!
	if (m1_sentno != m2_sentno) return results;
	int sentno = m1_sentno;

	PropositionSet *propSet = docTheory->getSentenceTheory(sentno)->getPropositionSet();
	MentionSet *mentionSet = docTheory->getSentenceTheory(sentno)->getMentionSet();
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		Proposition *prop = propSet->getProposition(p);
		bool found1 = false, found2 = false;
		for (int a = 0; a < prop->getNArgs(); a++) {
			if (prop->getArg(a)->getType() == Argument::MENTION_ARG) {
				if (prop->getArg(a)->getMention(mentionSet) == context.first)
					found1 = true;
				if (prop->getArg(a)->getMention(mentionSet) == context.second)
					found2 = true;
			}
			if (found1 && found2) {
				results.push_back(AttributeValuePair<const Proposition*>::create(SHARED_PROPOSITION_SYM, prop, getFullName()));
				break;
			}
		}
	}
	return results;
}
