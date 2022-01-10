// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "edt/ApposLinker.h"
#include "common/SessionLogger.h"

#include <iostream>

using namespace std;


ApposLinker::ApposLinker () {}

//NOTE: 
int ApposLinker::linkMention(LexEntitySet * currSolution, int currMentionUID, 
							 Entity::Type linkType, LexEntitySet *results[], int max_results) 
{
	LexEntitySet *newSet = currSolution->fork();
	Mention *currMention = currSolution->getMention(currMentionUID);
	results[0] = newSet;
	SessionLogger *logger = SessionLogger::logger;

	if(!Entity::isEDTType(currMention->entityType)) {
		logger->beginMessage();
		*logger << "ApposLinker::linkMention(): Appositive mention is not of an EDT type.";
		return 1;
	}

	Mention *first = currMention->getChild();
	Mention *linkableMention, *otherMention;
	if(isIndependentlyLinkable(first)) {
		linkableMention = first;
		if(isIndependentlyLinkable(first->getNext())) {
			logger->beginMessage();
			*logger << "ApposLinker::linkMention(): Both members of an appositive were marked independently linkable: "
				<< first->getMentionType() << " " << first->getNext()->getMentionType();
			return 1;
		}
		else otherMention = first->getNext();
	}
	else {
		if(!isIndependentlyLinkable(first->getNext())) {
			logger->beginMessage();
			*logger << "ApposLinker::linkMention(): Neither member of an appositive was marked independently linkable.";
			return 1;
		}
		else {
			linkableMention = first->getNext();
			otherMention = first;
		}
	}

	//now have otherMention link according to linkableMention's entity
	Entity *entity = newSet->getEntityByMention(linkableMention->getUID());
	if(entity == NULL) {
		logger->beginMessage();
		*logger << "ApposLinker::linkMention(): Supposedly independently-linkable appositive member has not been linked.";
		return 1;
	}
	else {
		newSet->add(otherMention->getUID(), entity->getID());
		newSet->add(currMention->getUID(), entity->getID());
		//newSet->add(
	}

//	cout << "ADDED to "<< entity->getID();
	return 1;
}

bool ApposLinker::isIndependentlyLinkable(Mention *mention) {
	//rules (in order of priority): 
	//	-anything not part of an appositive is linkable
	//	-anything part of an ignored appositive is linkable
	//	-pronoun is never linkable 
	//	-list is not linkable; paired-with-list is linkable
	//	-descriptor paired with name is never linkable
	//	-second descriptor of a descriptor-descriptor pair is not linkable
	//  -second name of a name-name pair is not linkable
	//	-everything else is linkable

	if(!mention->hasApposRelationship())
		return true;
	if(mention->mentionType == Mention::APPO)
		return false;
	if(!Entity::isEDTType(mention->getParent()->entityType))
		return true;
	Mention *other = mention->getNext();
	if(other == NULL)
		other = mention->getParent()->getChild();

	if(mention->mentionType == Mention::PRON)
		return false;
	if(mention->mentionType == Mention::LIST)
		return false;
	if(other->mentionType == Mention::LIST)
		return true;
	if(mention->mentionType == Mention::DESC) {
		if(other->mentionType == Mention::NAME)
			return false;
		else if(other->mentionType == Mention::DESC)
			return mention->getNext() != NULL;
	}
	if(mention->mentionType == Mention::NAME && other->mentionType == Mention::NAME) {
		return mention->getNext() != NULL;
	}
		
	return true;
}
	
