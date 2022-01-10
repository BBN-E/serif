#ifndef AR_RELATION_FINDER_H
#define AR_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"

#include "Generic/relations/RelationFinder.h"

class Parse;
class MentionSet;
class PropositionSet;
class EntitySet;
class ArabicP1RelationFinder;
class CorrectDocument;


class ArabicRelationFinder : public RelationFinder {
	friend class ArabicRelationFinderFactory;

public:

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

	// not used in Arabic for now
	void allowMentionSetChanges() {}
	void disallowMentionSetChanges() {}

	//CorrectDocument *currentCorrectDocument;

	enum { P1, NONE };

	~ArabicRelationFinder();

private:
	ArabicRelationFinder();

	ArabicP1RelationFinder *_p1RelationFinder;

	int mode;

};

// RelationFinder factory
class ArabicRelationFinderFactory: public RelationFinder::Factory {
	virtual RelationFinder *build() { return _new ArabicRelationFinder(); } 
};


#endif
