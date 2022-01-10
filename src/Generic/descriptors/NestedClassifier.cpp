// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/NestedClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
#include "Generic/common/ParamReader.h"

NestedClassifier::NestedClassifier() {
	_compoundMentionFinder = CompoundMentionFinder::getInstance();
	_replaceDescriptors = true;
	if (ParamReader::isParamTrue("use_pidf_desc")) {
		_replaceDescriptors = false;
	}
}

NestedClassifier::~NestedClassifier() {
}

int NestedClassifier::classifyMention(MentionSet *currSolution, 
	Mention *currMention, MentionSet *results[], int max_results,
	bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if
	// we're isBranching	
	if (isBranching)
		results[0] = _new MentionSet(*currSolution);
	else
		results[0] = currSolution;

	// can't claaify if the mention was made partitive or appositive or list
	if (currMention->mentionType == Mention::PART ||
		currMention->mentionType == Mention::LIST ||
		currMention->mentionType == Mention::APPO)
	{
		return 1;
	}

	Mention *nestedMention =
		_compoundMentionFinder->findNestedMention(currSolution,
												  currMention);

	if (nestedMention == 0)
		return 1;
	// must be a name or pron or desc
	if (!(nestedMention->mentionType == Mention::NAME ||
		  (nestedMention->mentionType == Mention::NONE &&
		   nestedMention->getParent() != 0 &&
		   nestedMention->getParent()->mentionType == Mention::NAME) ||
		/* that second test is there because a NAME might have been
		   turned into a NONE if it is was already found as a
		   nested mention of a different node */
		  nestedMention->mentionType == Mention::PRON ||
		  nestedMention->mentionType == Mention::DESC))
	{
		return 1;
	}

	// the forked mention
	Mention* newMention = results[0]->getMention(currMention->getIndex());
	// to be safe, if we're branching, zero out the former set and mention so
	// we don't accidentally refer to it again
	if (isBranching) {
		currSolution = 0;
		currMention = 0;
		nestedMention = 0;
	}
	// the forked nested mention
	Mention* newNestedMention = 
		_compoundMentionFinder->findNestedMention(results[0],
												  newMention);
	if (newNestedMention == 0) {
		throw InternalInconsistencyException(
			"NestedClassifier::classifyMention()",
			"forked mention set doesn't have nested!");
	}


	if (newNestedMention->mentionType == Mention::NAME ||
		newNestedMention->mentionType == Mention::NONE)
		/* it's NONE if it's a nested name of a different node */
	{
		// for names, kill nested mention but refer to it as this
		// mention's head
		newNestedMention->mentionType = Mention::NONE;
		newMention->mentionType = Mention::NAME;
		newMention->setEntityType(newNestedMention->getEntityType());

		// in case nested mention was already found as nested in a 
		// mention below this one, make that intermediate mention a NONE
		if (newNestedMention->getParent() != 0) {
			if (newNestedMention->getParent()->getChild() != newNestedMention ||
				newNestedMention->getNext() != 0)
			{
				SessionLogger::warn("mention_set_creation") << "Nested Mention has Mention siblings.\n";
			}
			else {
				newNestedMention->getParent()->mentionType = Mention::NONE;
				newNestedMention->makeOrphan();

				newNestedMention->makeOnlyChildOf(newMention);
			}
		}
		else {
			newNestedMention->makeOnlyChildOf(newMention);
		}
	}
	else if (newNestedMention->mentionType == Mention::PRON) {
		// for pronouns, move nested mention up to current node
		newNestedMention->mentionType = Mention::NONE;
		newMention->mentionType = Mention::PRON;
		newMention->setEntityType(newNestedMention->getEntityType());

		// BTW, unlike names, there's no need to bother with
		// the parent-child relationship thing.
	}
	else if (newNestedMention->mentionType == Mention::DESC) {
		// for descriptors
		if(_replaceDescriptors){
			newNestedMention->mentionType = Mention::NONE;
			// In the DescriptorRecognizer, we will also desc-classify this mention
			// if the type is OTH.
		}
		else{
			//newNestedMention->mentionType = Mention::NONE;
			//newMention->mentionType = Mention::DESC;
			//newMention->setEntityType(newNestedMention->getEntityType());
		}
	}
	else {
		throw InternalInconsistencyException(
			"NestedClassifier::classifyMention()",
			"Unknown mention type encountered");
	}
	return 1;
}




