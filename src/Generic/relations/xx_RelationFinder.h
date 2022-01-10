#ifndef XX_RELATION_FINDER_H
#define XX_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/RelMentionSet.h"

class Parse;
class MentionSet;
class EntitySet;
class PropositionSet;
class CorrectDocument;
class RelationFinder;

class DefaultRelationFinder : public RelationFinder {
	friend class DefaultRelationFinderFactory;

public:
	void resetForNewSentence(DocTheory *docTheory, int sentence_num) {}
	void cleanup() {}

	using RelationFinder::getRelationTheory;

	RelMentionSet *getRelationTheory(const Parse *parse,
			                               MentionSet *mentionSet,
			                               EntitySet *entitySet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse)
	{
		return _new RelMentionSet();
	}

	RelMentionSet *getRelationTheory(EntitySet *entitySet, 
										   const Parse *parse,
			                               MentionSet *mentionSet,
										   ValueMentionSet *valueMentionSet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse = 0,
										   const PropTreeLinks* ptLinks = 0)
	{
		return _new RelMentionSet();
	}

	RelMentionSet *getRelationTheory(const Parse *parse,
			                               MentionSet *mentionSet,
										   ValueMentionSet *valueMentionSet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse = 0 ) 	
	{
		return _new RelMentionSet();
	}

	void allowMentionSetChanges() {}
	void disallowMentionSetChanges() {}


	CorrectDocument *currentCorrectDocument;

private:
	DefaultRelationFinder() {}

};

// RelationFinder factory
class DefaultRelationFinderFactory: public RelationFinder::Factory {
	virtual RelationFinder *build() { return _new DefaultRelationFinder(); } 
};

#endif
