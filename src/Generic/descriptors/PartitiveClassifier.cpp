// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/PartitiveClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

PartitiveClassifier::PartitiveClassifier() {
	_compoundMentionFinder = CompoundMentionFinder::getInstance();
}

PartitiveClassifier::~PartitiveClassifier() {
}

int PartitiveClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	Mention *partitiveWhole = 
		_compoundMentionFinder->findPartitiveWholeMention(currSolution,
														  currMention);
	// simply return the copy of the solution if no match
	if (partitiveWhole == 0)
		return 1;
	// the forked mention
	Mention* newMention = results[0]->getMention(currMention->getIndex());
	// to be safe, if we're branching, zero out the former set and mention so
	// we don't accidentally refer to it again
	if (isBranching) {
		currSolution = 0;
		currMention = 0;
		partitiveWhole = 0;
	}
	// the forked appositives
	Mention* newPartitiveWhole = 
		_compoundMentionFinder->findPartitiveWholeMention(results[0],
														  newMention);
	if (newPartitiveWhole == 0)
		throw InternalInconsistencyException("PartitiveClassifier::classifyMention()",
		"forked mention set doesn't have partitive!");

	// make sure there's no extra populated mention
	const SynNode* node = newMention->node;
	if (node->getHead()->hasMention()) {
		Mention *nestedMention = 
			results[0]->getMentionByNode(node->getHead());
			nestedMention->mentionType = Mention::NONE;
	}
	//if the child or parent is already populated, we get in exception,
	//bad np chunk parses can lead to this, ignore the partitive in these cases -mrf
	if((newMention->getChild() ==0) && newPartitiveWhole->getParent()==0){
		newMention->mentionType = Mention::PART;	
		newMention->setEntityType(newPartitiveWhole->getEntityType());
		newPartitiveWhole->makeOnlyChildOf(newMention);
	}	
	return 1;
	

}

