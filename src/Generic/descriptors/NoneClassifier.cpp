// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/NoneClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

NoneClassifier::NoneClassifier() {
}

NoneClassifier::~NoneClassifier() {
}

int NoneClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching	
	if (isBranching) {
		results[0] = _new MentionSet(*currSolution);
		currMention = results[0]->getMention(currMention->getIndex());
	}
	else
		results[0] = currSolution;

	// can't classify if the mention is NAME
	// don't need to if it's NONE
	// return the set copied, but unchanged
	switch (currMention->mentionType) {
		case Mention::NAME:
		case Mention::NONE:
		case Mention::PART:
		case Mention::APPO:
		case Mention::LIST:
		case Mention::NEST:
			return 1;
		default:
			break;
	}
	// otherwise, just make the mention NONE
	currMention->mentionType = Mention::NONE;
	return 1;
}


