// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef FAMILY_RELATION_FINDER_H
#define FAMILY_RELATION_FINDER_H

#include <boost/shared_ptr.hpp>


class RelMention;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class PropositionSet;
class RelMentionSet;
class PropTreeLinks;
class SentenceTheory;
class DocTheory;

class FamilyRelationFinder {
public:
	/** Create and return a new FamilyRelationFinder. */
	static FamilyRelationFinder *build() { return _factory()->build(); }
	/** Hook for registering new FamilyRelationFinder factories */
	struct Factory { virtual FamilyRelationFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	// constructor uses list of params to initialize

	virtual void resetForNewSentence() = 0;
	virtual RelMention *getFixedFamilyRelation(RelMention *rm, DocTheory *dt) = 0;

	virtual ~FamilyRelationFinder() {}

protected:
		FamilyRelationFinder() {}

private:
	static boost::shared_ptr<Factory> &_factory();


};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/relations/en_FamilyRelationFinder.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Generic/relations/xx_FamilyRelationFinder.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Generic/relations/xx_FamilyRelationFinder.h"
//#else
//	#include "Generic/relations/xx_FamilyRelationFinder.h"
//#endif

#endif
