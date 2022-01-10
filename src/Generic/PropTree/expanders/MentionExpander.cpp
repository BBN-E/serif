// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "Generic/PropTree/expanders/MentionExpander.h"
#include "Generic/PropTree/PropFactory.h"
#include <boost/foreach.hpp>
#include "theories/DocTheory.h"
#include "theories/Mention.h"



void MentionExpander::getAllMents(const Mention* ment, 
										 const DocTheory* dt,
										 std::vector<std::pair<const Mention*, float> >& allMents)
{
	const EntitySet* eset=dt->getEntitySet();

	// Extract all mentions we *might* want to expand to
	const Entity * ent = eset->getEntityByMention(ment->getUID());

	// ... collect all entity NAME mentions...
	for(int m_it = 0; ent && m_it != ent->getNMentions(); m_it++){
		const Mention * c_ment = eset->getMention(ent->getMention(m_it));
		if(c_ment->getMentionType() == Mention::NAME)
			allMents.push_back(std::make_pair(c_ment, mentionExpansionCost(ment, c_ment)));
	}
	// ... failing that, if we're a PRON or PART, collect entity DESC mentions
	if(allMents.empty() && (ment->getMentionType() == Mention::PRON || ment->getMentionType() == Mention::PART))
	{
		for(int m_it = 0; ent && m_it != ent->getNMentions(); m_it++){
			const Mention * c_ment = eset->getMention(ent->getMention(m_it));
			if(c_ment->getMentionType() == Mention::DESC)
				allMents.push_back(std::make_pair(c_ment, mentionExpansionCost(ment, c_ment)));
		}
	}
	// ... failing that, if we're a PRON, collect PART mentions
	if(allMents.empty() && ment->getMentionType() == Mention::PRON)
	{
		for(int m_it = 0; ent && m_it != ent->getNMentions(); m_it++){
			const Mention * c_ment = eset->getMention(ent->getMention(m_it));
			if(c_ment->getMentionType() == Mention::PART)
				allMents.push_back(std::make_pair(c_ment, mentionExpansionCost(ment, c_ment)));
		}
	}
}


void MentionExpander::addMentionPredicate(const PropNode_ptr& node, const Mention * ment, 
																float weight, bool lc)
{
	Symbol m_pred = PropFactory::mentionPredicateString(node->getDocTheory(), ment, lc).c_str();
	if(Predicate::validPredicate(m_pred))
		node->addPredicate(Predicate(Predicate::mentionPredicateType(ment), m_pred), weight);
}


