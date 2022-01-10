#ifndef es_RELATION_FINDER_H
#define es_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"

#include "Generic/relations/RelationFinder.h"


class Parse;
class MentionSet;
class ValueMentionSet;
class PropositionSet;
class EntitySet;
class SpanishComboRelationFinder;
class CorrectDocument;
class SentenceTheory;
class PatternRelationFinder;

class SpanishRelationFinder : public RelationFinder {
	friend class SpanishRelationFinderFactory;

public:
	~SpanishRelationFinder();
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
	SpanishRelationFinder();

	PatternRelationFinder *_patternRelationFinder;
	SpanishComboRelationFinder *_comboRelationFinder;
};

// RelationFinder factory
class SpanishRelationFinderFactory: public RelationFinder::Factory {
	virtual RelationFinder *build() { return _new SpanishRelationFinder(); } 
};


#endif
