// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "AggressiveMentionExpander.h"
#include "PropTreeExpander.h"
#include "../PropFactory.h"
#include "../Predicate.h"
#include <boost/foreach.hpp>

#include "theories/Mention.h"
#include "theories/DocTheory.h"


void AggressiveMentionExpander::expand(const PropNodes& pnodes) const {
	using namespace boost;	
	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		const Mention* ment=node->getMention();
		const DocTheory* dt=node->getDocTheory();

		if (!ment || !dt) {
			continue;
		}

		std::vector<std::pair<const Mention*, float> > allMents;
		MentionExpander::getAllMents(ment, dt, allMents);

		for(size_t m_ind = 0; m_ind != allMents.size(); ++m_ind)
		{
			const Mention* m=allMents[m_ind].first;
			const float weight=allMents[m_ind].second;

			addMentionPredicate(node, m, weight, false);

			if (MentionExpander::doesLowercasing() && m->getMentionType()!=Mention::NAME) {
				addMentionPredicate(node, m, weight, true);
			}
		}
	}
}


