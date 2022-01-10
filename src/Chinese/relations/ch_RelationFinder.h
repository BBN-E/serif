#ifndef CH_RELATION_FINDER_H
#define CH_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"

#include "Generic/relations/RelationFinder.h"

class Parse;
class MentionSet;
class ValueMentionSet;
class PropositionSet;
class EntitySet;
class ChineseMaxEntRelationFinder;
class ChineseP1RelationFinder;
class ChineseComboRelationFinder;
class OldMaxEntRelationFinder;
class CorrectDocument;


class ChineseRelationFinder : public RelationFinder {
	friend class ChineseRelationFinderFactory;

public:
	~ChineseRelationFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);
	void cleanup();

	using RelationFinder::getRelationTheory;

	RelMentionSet *getRelationTheory(const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet,
								   const Parse *secondaryParse) 
	{
		return getRelationTheory(0, parse, mentionSet, valueMentionSet, propSet, secondaryParse);
	}

	RelMentionSet *getRelationTheory(EntitySet *entitySet,
									const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet,
								   const Parse *secondaryParse,
								   const PropTreeLinks* ptLinks = 0);



	//CorrectDocument *currentCorrectDocument;

	enum { P1, MAXENT, COMBO, OLD_MAXENT, NONE };

	void allowMentionSetChanges();
	void disallowMentionSetChanges();


private:
	ChineseRelationFinder();

	ChineseMaxEntRelationFinder *_maxEntRelationFinder;
	OldMaxEntRelationFinder *_oldMaxEntRelationFinder;
	ChineseP1RelationFinder *_p1RelationFinder;
	ChineseComboRelationFinder *_comboRelationFinder;

	int mode;

};

// RelationFinder factory
class ChineseRelationFinderFactory: public RelationFinder::Factory {
	virtual RelationFinder *build() { return _new ChineseRelationFinder(); } 
};


#endif
