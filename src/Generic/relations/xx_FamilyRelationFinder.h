#ifndef XX_FAMILY_RELATION_FINDER_H
#define XX_FAMILY_RELATION_FINDER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/RelMention.h"
#include "Generic/relations/FamilyRelationFinder.h"


class RelMention;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class PropositionSet;
class RelMentionSet;
class PropTreeLinks;
class SentenceTheory;
class DocTheory;

class DefaultFamilyRelationFinder : public FamilyRelationFinder {
private:
	friend class DefaultFamilyRelationFinderFactory;
public:
	virtual void resetForNewSentence() {};

	//using FamilyRelationFinder::getFixedFamilyRelation;

	RelMention *getFixedFamilyRelation(RelMention *rm, DocTheory *dt) 
	{
		return _new RelMention(*rm);
	}

private:
	DefaultFamilyRelationFinder() {}

};


// RelationFinder factory
class DefaultFamilyRelationFinderFactory: public FamilyRelationFinder::Factory {
	virtual FamilyRelationFinder *build() { return _new DefaultFamilyRelationFinder(); } 
};
 
#endif
