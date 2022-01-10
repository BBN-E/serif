// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "ConservativeMentionExpander.h"
#include "../Predicate.h"
#include "../PropNode.h"
#include "../PropFactory.h"

#include <boost/foreach.hpp>

#include "theories/Mention.h"
#include "theories/DocTheory.h"
#include "theories/SynNode.h"


void ConservativeMentionExpander::expand(const PropNodes& pnodes) const {
	using namespace boost;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		const Mention* ment=node->getMention();
		const DocTheory* dt=node->getDocTheory();

		if (!ment || !dt) {
			return;
		}

		std::vector<std::pair<const Mention*, float> > allMents;
		MentionExpander::getAllMents(ment, dt, allMents);

		// Expand to the token-closest mention
		size_t best_distance  = -1;
		const Mention * best_ment = 0;
		float best_ment_weight = 0.0f;

		for(size_t m_ind = 0; m_ind != allMents.size(); ++m_ind)
		{
			const Mention * c_ment = allMents[m_ind].first;

			// determine which mention is the antecedent of the other
			const Mention * left_ment = 0, * right_ment = 0;
			if(  c_ment->getSentenceNumber() <  ment->getSentenceNumber() ||
				(c_ment->getSentenceNumber() == ment->getSentenceNumber() &&
				c_ment->getHead()->getStartToken() < ment->getHead()->getStartToken()) )
			{
				left_ment  = c_ment;
				right_ment = ment;
			} else {
				left_ment  = ment;
				right_ment = c_ment;
			}

			// compute the mention token distance
			size_t cur_distance = 0;
			for(int cur_sent = left_ment->getSentenceNumber();
				cur_distance < best_distance && cur_sent <= right_ment->getSentenceNumber(); ++cur_sent)
			{
				size_t min_tok = 0, max_tok = dt->getSentenceTheory(cur_sent)->getTokenSequence()->getNTokens();

				if(cur_sent == left_ment->getSentenceNumber())
					min_tok = left_ment->getHead()->getStartToken();
				if(cur_sent == right_ment->getSentenceNumber())
					max_tok = right_ment->getHead()->getStartToken();

				cur_distance += max_tok - min_tok;
			}

			// keep the minimum-distance mention
			if( cur_distance < best_distance ){
				best_ment = c_ment;
				best_ment_weight = allMents[m_ind].second;
				best_distance = cur_distance;
			}
		}

		// didn't find a mention to expand to?
		if(!best_ment) return;

		addMentionPredicate(node, best_ment, best_ment_weight, false);

		if (MentionExpander::doesLowercasing() && best_ment->getMentionType()!=Mention::NAME) {
			addMentionPredicate(node, best_ment, best_ment_weight, true);
		}
	}
}

