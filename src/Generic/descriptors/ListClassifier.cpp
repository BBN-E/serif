// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/ListClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

ListClassifier::ListClassifier() {
	_compoundMentionFinder = CompoundMentionFinder::getInstance();
}

ListClassifier::~ListClassifier() {
}

int ListClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	// can't classify if the mention was made partitive or appositive
	if (currMention->mentionType == Mention::PART ||
		currMention->mentionType == Mention::APPO)
	{
        return 1;
	}

	Mention **listMembers =
		_compoundMentionFinder->findListMemberMentions(currSolution,
													   currMention);
	if (listMembers == 0)
		return 1;
	// the forked mention
	Mention* newMention = results[0]->getMention(currMention->getIndex());
	// to be safe, if we're branching, zero out the former set and mention so
	// we don't accidentally refer to it again
	if (isBranching) {
		currSolution = 0;
		currMention = 0;
		listMembers = 0;
	}
	// the forked list members
	Mention** newListMembers = 
		_compoundMentionFinder->findListMemberMentions(results[0],
													   newMention);
	if (newListMembers == 0)
		throw InternalInconsistencyException("ListClassifier::classifyMention()",
		"forked mention set doesn't have list!");

	newMention->mentionType = Mention::LIST;

	newListMembers[0]->makeOnlyChildOf(newMention);

	// Speculate that the entity type is the type of the first
	// mention in the member list. If we come across a member
	// with a different type, then we'll just call it OTH
	// CHANGE 1/31 EMB: we don't let UNDET change the type of a list
	EntityType eType = newListMembers[0]->getEntityType();
	int i = 1;
	while (newListMembers[i] != 0) {
		newListMembers[i]->makeNextSiblingOf(newListMembers[i-1]);

		if (eType != newListMembers[i]->getEntityType()) {
			if (eType.isDetermined() &&
				newListMembers[i]->getEntityType().isDetermined()) {
				eType = EntityType::getOtherType();
				//break; JCS - 6/27 - allow all OTH list members to be added
			} else if (!eType.isDetermined()) {
				eType = newListMembers[i]->getEntityType();
			}
		}

		i++;
	}

	newMention->setEntityType(eType);

	return 1;

}


