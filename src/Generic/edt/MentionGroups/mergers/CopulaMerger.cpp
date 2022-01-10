// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/CopulaMerger.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Proposition.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

#include <boost/foreach.hpp>

CopulaMerger::CopulaMerger(MentionGroupConstraint_ptr constraints) : PairwiseMentionGroupMerger(Symbol(L"Copula"), constraints) {}

bool CopulaMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {

	if ((m1->getMentionType() == Mention::NAME || m1->getMentionType() == Mention::NEST) && 
		(m2->getMentionType() == Mention::NAME || m2->getMentionType() == Mention::NEST))
		return false;

	if (!m1->isOfRecognizedType() || !m2->isOfRecognizedType())
		return false;

	std::vector<AttributeValuePair_ptr> features = cache.getMentionPairFeaturesByName(m1, m2, Symbol(L"MentionPair-shared-proposition"), Symbol(L"shared-proposition"));
	BOOST_FOREACH(AttributeValuePair_ptr f, features) {
		boost::shared_ptr< AttributeValuePair<const Proposition*> > p;
		if ((p = boost::dynamic_pointer_cast< AttributeValuePair<const Proposition*> >(f))) {
			const Proposition *prop = p->getValue();
			const MentionSet *mentionSet = m1->getMentionSet();
			if (prop->getPredType() == Proposition::COPULA_PRED) {

				if (prop->getNegation() != 0)
					continue;

				if (prop->getNArgs() < 2 ||
					prop->getArg(0)->getType() != Argument::MENTION_ARG ||
					prop->getArg(1)->getType() != Argument::MENTION_ARG)
				{
					SessionLogger::dbg("mention_groups") << "Copula has fewer than 2 args\n";
					continue;
				}

				if ((prop->getArg(0)->getMention(mentionSet) == m1 && prop->getArg(1)->getMention(mentionSet) == m2) ||
					(prop->getArg(0)->getMention(mentionSet) == m2 && prop->getArg(1)->getMention(mentionSet) == m1)) 
				{
					if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
						SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of "
							<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
							<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
						SessionLogger::dbg("MentionGroups_shouldMerge") << "COPULA FOUND";
					}
					return true;
				}
			}
		}
	}
	return false;
}
