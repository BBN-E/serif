#ifndef EN_RELATION_FINDER_H
#define EN_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"

#include "Generic/relations/RelationFinder.h"


class Parse;
class MentionSet;
class ValueMentionSet;
class PropositionSet;
class EntitySet;
class EnglishComboRelationFinder;
class CorrectDocument;
class SentenceTheory;
class PatternRelationFinder;

class EnglishRelationFinder : public RelationFinder {
	friend class EnglishRelationFinderFactory;

public:
	~EnglishRelationFinder();
	void cleanup();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);
	void allowMentionSetChanges();
	void disallowMentionSetChanges();

	RelMentionSet *getRelationTheory(EntitySet *entitySet, const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
								   PropositionSet *propSet,
								   const Parse *secondaryParse = 0,
								   const PropTreeLinks *ptLink = 0)
	{
		return getRelationTheory(entitySet, 0, parse, mentionSet, valueMentionSet, propSet, secondaryParse);
	}

	RelMentionSet *getRelationTheory(EntitySet *entitySet, SentenceTheory *sentTheory, 
									const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
								   PropositionSet *propSet,
								   const Parse *secondaryParse = 0,
								   const PropTreeLinks *ptLink = 0);

	RelMentionSet *getRelationTheory(const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
								   PropositionSet *propSet,
								   const Parse *secondaryParse = 0)
	{
		return getRelationTheory(0, 0, parse, mentionSet, valueMentionSet, propSet, secondaryParse);
	}

	//CorrectDocument *currentCorrectDocument;
	
private:
	EnglishRelationFinder();

	PatternRelationFinder *_patternRelationFinder;
	EnglishComboRelationFinder *_comboRelationFinder;
};

// RelationFinder factory
class EnglishRelationFinderFactory: public RelationFinder::Factory {
	virtual RelationFinder *build() { return _new EnglishRelationFinder(); } 
};


#endif
