// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Arabic/edt/ar_DescLinker.h"

//Always create a new entity
int ArabicDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, 
							 LexEntitySet *results[], int max_results) 
{ 
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = 0;

	guess = _guessNewEntity(currMention, linkType);
		// only one branch here - desc linking is deterministic
	LexEntitySet* newSet = currSolution->fork();
	if (guess->id == EntityGuess::NEW_ENTITY) {
		newSet->addNew(currMention->getUID(), guess->type);
	}
	results[0] = newSet;

	// MEMORY: a guess was definitely created above, and is removed now.
	delete guess;
	return 1;
}

EntityGuess* ArabicDescLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}
