// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_EVENT_FINDER_H
#define PATTERN_EVENT_FINDER_H

class Parse;
class SynNode;
class TokenSequence;
class ValueMentionSet;
class MentionSet;
class PropositionSet;
class EventMentionSet;
class DocTheory;
class SentenceTheory;
class EventMention;

#include "Generic/patterns/PatternSet.h"

class PatternEventFinder {
public:
	PatternEventFinder();
	~PatternEventFinder() {}

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);

	EventMentionSet *getEventTheory(const Parse* parse);

private:

	DocTheory *_docTheory;
	int _sentence_number;
	std::vector<PatternSet_ptr> _patternSets;
	bool _splitListMentions;

	void addEventMentions(SentenceTheory *sTheory, PatternSet_ptr patternSet, EventMentionSet *emSet);
	void addEventType(EventMention *vm, std::wstring event_type);
	void addAnchor(EventMention *vm, const SynNode *node, SentenceTheory *sTheory);


};


#endif
