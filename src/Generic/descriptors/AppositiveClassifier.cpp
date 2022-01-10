// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/AppositiveClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

AppositiveClassifier::AppositiveClassifier() {
	_compoundMentionFinder = CompoundMentionFinder::getInstance();
}

AppositiveClassifier::~AppositiveClassifier() {
}

int AppositiveClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	// can't classify if the mention was made partitive
	// return the set copied, but unchanged
	if (currMention->mentionType == Mention::PART)
		return 1;

	Mention **apposMembers =
		_compoundMentionFinder->findAppositiveMemberMentions(currSolution,
															 currMention);
	// simply return the copy of the solution if no match
	if (apposMembers == 0)
		return 1;


	// the forked mention
	Mention* newMention = results[0]->getMention(currMention->getIndex());
	// to be safe, if we're branching, zero out the former set and mention so
	// we don't accidentally refer to it again
	if (isBranching) {
		currSolution = 0;
		currMention = 0;
		apposMembers = 0;
	}

	// the forked appositives
	Mention** newApposMembers = 
		_compoundMentionFinder->findAppositiveMemberMentions(results[0],
															 newMention);
	if (newApposMembers == 0)
		throw InternalInconsistencyException("AppositiveClassifier::classifyMention()",
		"forked mention set doesn't have appositive!");

	// language-specific coercion of entity types
	// NOTE: mentions can be modified here!
	_compoundMentionFinder->coerceAppositiveMemberMentions(newApposMembers);
	// if the types don't match, abandon (no changes have been made)
	if (newApposMembers[0]->getEntityType() != newApposMembers[1]->getEntityType())
		return 1;

	// set mention and entity type for the node, and set the appositive structure
	newMention->mentionType = Mention::APPO;
	newMention->setEntityType(newApposMembers[0]->getEntityType());
	newApposMembers[0]->makeOnlyChildOf(newMention);
	int i = 1;
	while (newApposMembers[i] != 0) {
		newApposMembers[i]->makeNextSiblingOf(newApposMembers[i-1]);
		i++;
	}
	return 1;
}


