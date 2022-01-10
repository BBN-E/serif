// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_EVENT_FINDER_H
#define STAT_EVENT_FINDER_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"

class Parse;
class TokenSequence;
class ValueMentionSet;
class MentionSet;
class EntitySet;
class PropositionSet;
class EventMentionSet;
class DocTheory;
class EventMention;
class EventTriggerFinder;
class EventArgumentFinder;
class EventModalityClassifier;
class DTTagSet;

//#define AssignMODALITY

class StatEventFinder {
public:
	StatEventFinder();
	~StatEventFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);

	EventMentionSet *getEventTheory(const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet,
		Parse *parse,
		MentionSet *mentionSet,
		PropositionSet *propSet);

	void setDocumentTopic(Symbol topic);

	const DTTagSet * getEventTypeTagSet();
	const DTTagSet * getArgumentTypeTagSet();

private:
	int _sentence_num;
	EventMention **_potentials;

	EventTriggerFinder *_triggerFinder;
	EventArgumentFinder *_argumentFinder;
	EventModalityClassifier *_modalityClassifier;

};


#endif
