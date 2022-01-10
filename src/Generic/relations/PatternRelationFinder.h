// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_RELATION_FINDER_H
#define PATTERN_RELATION_FINDER_H

class Parse;
class SynNode;
class TokenSequence;
class ValueMentionSet;
class MentionSet;
class PropositionSet;
class EntitySet;
class RelMentionSet;
class DocTheory;
class SentenceTheory;
class RelMention;
class PropTreeLinks;

#include "Generic/patterns/PatternSet.h"

class PatternRelationFinder {
public:
	PatternRelationFinder();
	~PatternRelationFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);

	RelMentionSet *getRelationTheory();

private:

	DocTheory *_docTheory;
	int _sentence_number;
	std::vector<PatternSet_ptr> _patternSets;

};


#endif
