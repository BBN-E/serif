// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescriptorClassifierTrainer/OtherClassifier.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/common/InternalInconsistencyException.h"

OtherClassifier::OtherClassifier()  {}

OtherClassifier::~OtherClassifier() {}

int OtherClassifier::classifyMention (MentionSet *currSolution, 
									  Mention *currMention, 
							          MentionSet *results[], 
						    		  int max_results, 
									  bool isBranching)
{
	// only want to classify DESC and NONE. something else might get in via
	// nestedMentionClassifier. don't continue if it is
	switch (currMention->mentionType) {
		// bail case: reproduce the current solution and return as is
		case Mention::NAME:
		case Mention::PRON:
		case Mention::PART:
		case Mention::APPO:
		case Mention::LIST:
			if (isBranching)
				results[0] = _new MentionSet(*currSolution);
			else
				results[0] = currSolution;
			return 1;
		case Mention::NONE:
		case Mention::DESC:
			break;
		default:
			throw InternalInconsistencyException("DescriptorClassifier::classifyMention()",
				"Unexpected mention type seen");
	}

	// old style: mention belongs to solution, so just change the type, move the pointer,
	// and return
	if (!isBranching) {
		currMention->mentionType = Mention::DESC;
		results[0] = currSolution;
		results[0]->setDescScore(1);
	}
	// new style: fork the mention set, re-resolve the mention by id,
	else {
		MentionSet* newSet = _new MentionSet(*currSolution);
		// locate the newly forked mention, since that's what we're changing
		Mention* newMent = newSet->getMention(currMention->getIndex());
		newMent->mentionType = Mention::DESC;
		results[0] = newSet;
		results[0]->setDescScore(1);
	}
	
	return 1;
}

