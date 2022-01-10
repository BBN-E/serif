// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_FINDER_H
#define RELATION_FINDER_H

#include <boost/shared_ptr.hpp>

class Parse;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class PropositionSet;
class RelMentionSet;
class PropTreeLinks;
class SentenceTheory;
class DocTheory;
class CorrectDocument;

class RelationFinder {
public:
	/** Create and return a new RelationFinder. */
	static RelationFinder *build() { return _factory()->build(); }
	/** Hook for registering new RelationFinder factories */
	struct Factory { virtual RelationFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual void cleanup() = 0;
	virtual void resetForNewSentence(DocTheory *docTheory, int sentence_num) = 0;

	virtual RelMentionSet *getRelationTheory(EntitySet *entitySet, 
										   const Parse *parse,
			                               MentionSet *mentionSet,
										   ValueMentionSet *valueMentionSet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse = 0,
										   const PropTreeLinks* propTreeLinks = 0) 
		= 0;

	virtual RelMentionSet *getRelationTheory(const Parse *parse,
			                               MentionSet *mentionSet,
										   ValueMentionSet *valueMentionSet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse = 0 ) 
		= 0;

	/* 5/19/2010 [Edward Loper] This override was added because I needed to get access
	 * to the sentence theory (and in particular, to the token sequence) to find 
	 * token offsets for work related to text/Projects/SERIF/experiments/relation-confidence.
	 * I'm defining it to default to call the version that does not take a sentTheory
	 * argument, for backwards compatibility.  (note: why do we pass the mention set, 
	 * value mention set, etc in separately, when we could just pass the sentence theory?)
	 * Currently, en_RelationFinder is the only subclass of RelationFinder that 
	 * actually overrides this method.
	 */
	virtual RelMentionSet *getRelationTheory(EntitySet *entitySet, 
											SentenceTheory *sentTheory,
										   const Parse *parse,
			                               MentionSet *mentionSet,
										   ValueMentionSet *valueMentionSet,
			                               PropositionSet *propSet, 
										   const Parse* secondaryParse = 0,
										   const PropTreeLinks* propTreeLinks = 0) 
	{
		return getRelationTheory(entitySet, parse, mentionSet, valueMentionSet, propSet, secondaryParse, propTreeLinks);
	}

	CorrectDocument *currentCorrectDocument;

	virtual void allowMentionSetChanges() = 0;
	virtual void disallowMentionSetChanges() = 0;

	virtual ~RelationFinder() {}

protected:
	RelationFinder() {}

private:
	static boost::shared_ptr<Factory> &_factory(); 	


};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/relations/en_RelationFinder.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/relations/ch_RelationFinder.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/relations/ar_RelationFinder.h"
//#else
//	#include "Generic/relations/xx_RelationFinder.h"
//#endif

#endif
