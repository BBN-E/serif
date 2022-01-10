// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEPRECATED_PATTERN_EVENT_FINDER_H
#define DEPRECATED_PATTERN_EVENT_FINDER_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/events/patterns/EventSearchNode.h"

class CorrectAnswers;

class Parse;
class MentionSet;
class ValueMentionSet;
class PropositionSet;
class EventMentionSet;
class EventPatternMatcher;
class EventMention;
class DocTheory;
class EventLinker;
class SurfaceLevelSentence;


class DeprecatedPatternEventFinder {
public:
	DeprecatedPatternEventFinder();
	~DeprecatedPatternEventFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);

	EventMentionSet *getEventTheory(const Parse *parse,
		MentionSet *mentionSet,
		ValueMentionSet *valueMentionSet,
		const PropositionSet *propSet);

	static bool DEBUG;
	static UTF8OutputStream _debugStream;
	static Symbol BLOCK_SYM;
	static Symbol FORCED_SYM;

	static CorrectAnswers* _correctAnswers;
	static int _sentence_num;
	static Symbol _docName;

	bool _allow_mention_set_changes;
	void allowMentionSetChanges() { _allow_mention_set_changes = true; }
	void disallowMentionSetChanges() { _allow_mention_set_changes = false; }


private:
	EventPatternMatcher *_matcher;
	SurfaceLevelSentence *_surfaceSentence;
	Symbol TOPLEVEL_PATTERNS;

	bool _event_found[MAX_SENTENCE_TOKENS];

	EventMention *_events[MAX_SENTENCE_EVENTS];
	int _n_events;
	void cullDuplicateEvents();

	bool _use_correct_answers;

// EMB 6/22/05: This is commented out because I think we will never use it again,
//              so I am disinclined to try to figure out how to make it work in the
//              new coreference infrastructure. 
//	void cullIncorrectEvents(const MentionSet *mentionSet, EntitySet *entitySet);
//	void findMissingCorrectEvents(const PropositionSet *propSet, 
//		const MentionSet *mentionSet, EntitySet *entitySet);
};


#endif
